/* /proc device interface for MIB settings in  Nanoradio Linux WiFi driver. */
/* $Id: mib.c 16214 2010-09-28 07:17:05Z anbg $ */
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/netdevice.h>

#include "nanoutil.h"
#include "nanoparam.h"
#include "nanonet.h"
#include "wifi_engine.h"

#include "px.h"

static int nrx_mib_release(struct nrx_px_softc*, struct inode*, struct file*);

struct nrx_px_entry mib_px_entry = {
   .name = "mib", 
   .mode = S_IRUSR|S_IWUSR, 
   .blocksize = 1024, 
   .init = NULL,
   .open = NULL,
   .release = nrx_mib_release
};

static int
nano_mib_download(struct nrx_px_softc *psc)
{
   int status;
 
   if(nrx_px_size(psc) == 0) {
      KDEBUG(TRACE, "No Mib data available");
      nrx_px_runlock(psc);
      return -EINVAL;
   }
    
   KDEBUG(TRACE, "Sending Mibs");

   status =  WiFiEngine_SendMIBSetFromNvmem(nrx_px_data(psc),
                                            nrx_px_size(psc));
   return 0;
}

static int
nrx_mib_release(struct nrx_px_softc *psc, 
                struct inode *inode, 
                struct file *file)
{
   nano_mib_download(psc);

   return 0;
}
