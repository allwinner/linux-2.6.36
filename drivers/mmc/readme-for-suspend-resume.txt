linux内核mmc子系统中卡suspend/resume的相关流程分析

分三种情况：
1 standby过程中不拔卡
2 standby的过程中拔卡不换卡
3 standby的过程中拔卡并换卡

正常的流程会由于考虑到卡在standby的时候可能会被拔出，而在standby的时候是无法检测到卡拔插的，
事实上由于在设备standby的时候拔插卡这个动作很普遍也很正常，拔插动作的目的可能只是为了临时使
用卡到其他用途，所以没有必要在standby的过程中由于卡的拔插动作而唤醒系统，系统唤醒需要由唯一
指定的动作执行，这样简单而且目的明确。

通常情况
在standby的时候不判断卡的拔插状态，那么在系统resume的过程中就需要对卡进行有效的识别
当系统进入suspend的时候，pm操作流程如下：

	pm_prepare_console();
	pm_notifier_call_chain(PM_SUSPEND_PREPARE);
	usermodehelper_disable();
	suspend_freeze_processes();
	suspend_thaw_processes();
	usermodehelper_enable();
	pm_notifier_call_chain(PM_POST_SUSPEND);
	pm_restore_console();
	
可以看到pm程序在发出suspend_prepare的时候并没有冻结用户空间的进程，在这一命令发出后SD卡的一
系列流程如下：

  mmc的core层会对卡的操作进行同步处理，以保证队列在suspend的时候不至于丢失，在这里进入
  mmc_pm_notify函数执行PM_SUSPEND_PREPARE操作，再次过程中，如果sd/mmc bus从的ops支持suspend
  操作，那么PM_SUSPEND_PREPARE便不执行任何操作，但是如果bus层的ops不支持suspend操作的话
  那么PM_SUSPEND_PREPARE操作将强制性的remove掉卡，然后再一层一层的执行suspend操作，当resume
  的时候再去重新加载卡，此加载过程和插拔过程的加载一模一样，实现了对卡的完全的重新枚举的过程
  即不区分SD MMC SDIO种类一律重新识别。
  
  sd/mmc bus层的ops是否支持suspend操作在系统的配置中有一配置选项CONFIG_MMC_UNSAFE_RESUME可以
  控制，该选项假设卡在suspend/resume的过程中是不会remove的，从而在系统pm执行PM_SUSPEND_PREPARE
  的时候不再执行强制remove操作，但是如果不选择此选项，系统认为suspend/resume的过程中卡是有可能
  拔出的，因此会执行remove操作，而我们遇到的问题也是由remove的操作引起的。
  
  为了保护上层各种应用对卡控制器host的控制同步，mmc子系统通过两个函数实现对host单一访问的控制
  mmc_claim_host获取控制权，mmc_release_host释放控制权
  
目前的问题：
  系统内核mmc子系统没有选择CONFIG_MMC_UNSAFE_RESUME配置选项，此时当子系统收到PM_SUSPEND_PREPARE
  命令时执行remove操作，由于其对host有操作，因此在remove之前需要获取host的控制权，从而执行
  mmc_claim_host的操作，在remove的过程中block层会将卡对应的block设备删除，同时清除mmc请求队列，
  而删除block设备中的del_gendisk的请求会引起块设备层或文件系统对卡某些文件的访问，可能是清除buffer
  中的数据也可能是别的原因，总之对SD卡来说发起了新的request，此时block层正在进行mmc请求队列的清理
  操作，再此操作中发出stop_thread的命令，并唤醒mmc请求队列，而当mmc请求队列被唤醒同时未终止之时
  由于队列发现有新的请求进入，从而会去执行新的请求访问，此时mmc_rw_rq操作由于需要对host进行操作，
  因此需要获得host的控制权，从而执行mmc_claim_host操作，而由于remove之前的claim操作并没有release
  从而无法继续进行，这样mmc的队列请求无法执行下去，而线程再一次睡眠，并且因无法收到stop_thread
  的命令而不能终止。而remove函数中的cleanup_queue的操作也因无法等待线程结束而执行不下去，从而形成
  死锁。
  
此前的修改：
  此前猛哥对此块代码进行了细微的调整，暂时解决了死锁的问题，但是当resume的时候会出现卡无法挂载的情况
  此现象不定期出现，原因可能由于代码修改造成，但也可能由于别的原因造成。

他家公司的用法
  子系统的代码在每家公司中并没有差别，每家芯片厂商的代码差别仅仅在host操作上，他处并没有不同
  
问题的解决：
  使用内核配置CONFIG_MMC_UNSAFE_RESUME
  当使用内核配置CONFIG_MMC_UNSAFE_RESUME时，mmc子系统在bus操作注册的时候会分别针对响应的bus注册
  suspend/resume函数: mmc_sd_suspend/mmc_sd_resume， mmc_suspend/mmc_resume，sdio操作处理流程因
  不同而本身具有mmc_sdio_suspend/mmc_sdio_resume的操作。
  
  内核配置了CONFIG_MMC_UNSAFE_RESUME之后，mmc子系统会将卡标记成为mmc_assume_removable，即假设卡
  会拔插，在此标记后，当pm执行PM_SUSPEND_PREPARE操作时，core层发现bus层具有suspend操作便不再进行
  remove操作，从而不会出现死锁的现象。此后mmc子系统会对各层进行一一suspend操作，最终进入standby
  状态，流程如下：
  
  mmc_pm_notify->mmc_bus_suspend->mmc_queue_suspend->mmc_sd_suspend->mmc_suspend_host->host_controller_suspend
  
  在增加的mmc_bus_suspend操作中会对卡进行select和deselect操作，从而让卡在suspend之前进入自身的
  standby状态。
  
  resume流程相反，如下：
  host_controller_resume->mmc_resume_host->mmc_sd_resume->mmc_resume_host->mmc_bus_resume->mmc_queue_resume->mmc_pm_notify
  
  1） standby过程中不拔卡
  当系统resume的时候在完成controller到bus层的一系列resume的过程后，进入mmc_bus_resume过程中对卡
  进行重新初始化，此过程仅仅是一个初始化过程，而不同于之前的完全的枚举过程。初始化过后一层一层的
  resume，最终唤醒resume请求队列并通知pm管理系统，最后和suspend之前的上层的各个状态一一对应起来

  2）standby过程中拔卡不换卡
  如果在standby的过程中将卡拔出，在resume之前再插入的话，mmc_bus_resume的过程中对卡的初始化流程
  和1）中是一模一样的，卡同样能够正确的初始化，因此可以继续完成之后的操作。
  
  3）standby过程中拔卡并换卡插入
  如果在standby的过程中将卡拔出并换另一张卡插入，在resume之前再插入的话，mmc_bus_resume的过程中
  对卡进行同样的初始化流程便会失败，因为卡在识别过程中的信息发生了改变，从而mmc子系统认为卡在
  suspend之后发生了拔插操作，从而init失败。但由于suspend之前的mmc请求队列依然存在，此时并不对
  卡进行额外的操作，resume操作依次往下执行，直到resume_queue之后队列中的请求继续传输，此时由于
  卡已改变，前面剩余的请求便会失败，在尝试一系列retry操作之后，mmc子系统block层发现错误并报告
  上层，此时子系统对卡进行rescan操作，从而完成一次完全的枚举操作再次识别卡的结构并报告分区。
  
  在这三种情况下第3）中情况中由于卡被更换，造成之前应用程序的剩余访问发生错误，从而再次识别的时间
  较前两种情况稍微长一些。但此机制依然可以保证传输的正确进行。
  
有一点疑问：
  在网上查阅了大量的代码和案例，很多人也遇到了相同的情况，因此在此前使用过程中互锁的bug的存在是
  必然的，但是内核中却在各个版本中并未对此bug进行修复，而且也没有一个完美的patch发布，感觉很奇怪，
  此问题是否为bug，或是只是一种特殊应用，或是什么原因不太明白，这一点可能需要深入的研究一下。


附加：
MMC_BLOCK_DEFERRED_RESUME的说明

在本内核中还有一个宏MMC_BLOCK_DEFERRED_RESUME和suspend/resume操作有关，此宏定义了一种方式，当
mmc卡进入到suspend的时候，如果定义了MMC_BLOCK_DEFERRED_RESUME，那么对卡的flag做一个标记，表明
下次resume的时候需要手动resume（manual_resume），而在系统resume的时候检查到此标记直接跳过，不
进行任何操作，真正的resume发生在文件的读写时，这样如果系统恢复之后不对卡进行任何操作的话便不会
发生卡的resume操作，可以达到节省卡的resume流程和在恢复后不操作的情况下节省一点卡的耗电。

当block层发出数据传输请求之前首先对resume标志进行判断，如果需要manual_resume操作，那么进行手动
resume，这个行为发生在系统resume之后的第一次数据传输请求时，此时mmc子系统对卡进行识别，并和之前
的卡的信息进行对比，如果卡的信息相同，即suspend的之后没有换过卡（拔卡无所谓），那么识别操作成功，
同时清除manual_resume的标志，如果suspend之后换了别的卡，那么识别过程失败，然后上层请求失败，但是
此时block层进队卡进行detect操作，通过发送命令的方式，由于卡并未init成功，因此detect操作失败，此时
block层保持现有状态不变，也不会umount卡，应用程序停在一个特定的位置：无法正确的识别卡上的内容。

很有意思的是，如果系统再次进入suspend，这时将卡再换回第一次suspend之前的卡，当系统resume之后请求
数据访问时，block层因卡信息比对正确而再次成功加载卡到系统，此时应用程序依然能够正常访问卡上文件。

所以可以看出，此种优化仅仅只针对那种卡不会拔出或拔出后不会更换其他卡的情况，因此在我们的系统中
如果要满足standby的情况下换卡的情况，就不太合适了，除非用户在看到应用程序报告访问文件失败后将
卡拔出并再次插入，以卡检测机制明确的通知block层卡更换的行为方可再次访问文件。

同时，当系统standby之后更换其他卡之后，再次resume的时候浏览器中显示的是之前卡上的文件列表而不是
新卡上的列表对用户来说也不太正常，造成一种错觉，也是不好的。

结论：MMC_BLOCK_DEFERRED_RESUME不可选