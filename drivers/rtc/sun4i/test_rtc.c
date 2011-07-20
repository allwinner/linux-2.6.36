#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

static int read_rtc()
{
    int rtc;
    struct rtc_time tm;

    rtc = open ( "/dev/rtc0", O_RDONLY );
    if (!rtc) {
        printf ( "Could not access RTC\n" );
    }
    memset ( &tm, 0, sizeof( struct rtc_time ));

    if(ioctl(rtc, RTC_RD_TIME, &tm))
        printf ( "Could not read time from RTC\n" );

    printf("read the rtc time: year = %d; mon = %d; mday = %d;\
            hour = %d; min = %d; hour = %d. \n", tm.tm_year, tm.tm_mon, tm.tm_mday, \
            tm.tm_hour, tm.tm_min, tm.tm_hour);
    close ( rtc );

    return 0;
}

typedef struct TIME_T{
    int year;
    int mon;
    int mday;
    int hour;
    int min;
    int sec;
}time_test;

static void write_rtc(time_test t)
{
    int rtc;
    struct rtc_time tm;

    rtc = open ( "/dev/rtc0", O_WRONLY );
    if (!rtc) {
        printf ( "Could not access RTC\n" );
    }
    
    tm.tm_year = t.year;
    tm.tm_mon  = t.mon;
    tm.tm_mday = t.mday;
    tm.tm_hour = t.hour;
    tm.tm_min  = t.min;
    tm.tm_sec  = t.sec;
    
//-------------------------------------------
    tm. tm_isdst = 0;
    
    printf("set the rtc time to: year = %d; mon = %d; mday = %d;\
            hour = %d; min = %d; hour = %d. \n", tm.tm_year, tm.tm_mon, tm.tm_mday, \
            tm.tm_hour, tm.tm_min, tm.tm_hour);
    if ( ioctl ( rtc, RTC_SET_TIME, &tm ))
        printf ( "Could not set the RTC time\n" );

    close ( rtc );
}



static int get_alarm()
{
    int rtc;
    struct rtc_time alrm;

    rtc = open ( "/dev/rtc0", O_RDONLY );
    if (!rtc) {
        printf ( "Could not access RTC\n" );
    }
    memset ( &alrm, 0, sizeof(struct rtc_time));

    if(ioctl(rtc, RTC_ALM_READ, &alrm))
        printf ( "Could not get alrm\n" );

    
    close ( rtc );

    return 0;
}

static int set_alarm()
{
    int rtc;    
    struct rtc_time alrm;

    rtc = open ("/dev/rtc0", O_WRONLY);
    if (!rtc) {
        printf ( "Could not access RTC\n" );
    }
    
    alrm.tm_mday = 1;
    alrm.tm_hour = 0;
    alrm.tm_min  = 0;
    alrm.tm_sec  = 20;

//-------------------------------------------
    if (ioctl (rtc, RTC_ALM_SET, &alrm))
        printf ( "Could not set alrm\n" );

    close ( rtc );
    return 0;

}


void test_rtc(time_test t)
{
    time_test tmp_t;

    tmp_t.year = t.year;
    tmp_t.mon  = t.mon;
    tmp_t.mday = t.mday;
    tmp_t.hour = t.hour;
    tmp_t.min  = t.min;
    tmp_t.sec  = t.sec;  

    write_rtc(tmp_t);
    sleep(2);
    read_rtc();
}

void test_rtc_all()
{
    time_test t;

   /*不再范围*/
    t.year = 109;
    t.mon  = 11;
    t.mday = 19;
    t.hour = 17;
    t.min  = 44;
    t.sec  = 01;

    test_rtc(t);
    sleep(1);

    /*正常日期点*/
    t.year = 110;
    t.mon  = 11;
    t.mday = 31;
    t.hour = 23;
    t.min  = 59;
    t.sec  = 58;

    test_rtc(t);
    sleep(1);

    /*不是闰年*/
    t.year = 110;
    t.mon  = 01;
    t.mday = 28;
    t.hour = 23;
    t.min  = 59;
    t.sec  = 58;

    test_rtc(t);
    sleep(1);

    /*闰年*/
    t.year = 112;
    t.mon  = 01;
    t.mday = 28;
    t.hour = 23;
    t.min  = 59;
    t.sec  = 58;

    test_rtc(t);
    sleep(1);

    /*日期临界点*/
    t.year = 163;
    t.mon  = 11;
    t.mday = 31;
    t.hour = 23;
    t.min  = 59;
    t.sec  = 58;
    
    test_rtc(t);
    sleep(1);

    /*超过63年*/
    t.year = 164;
    t.mon  = 02;
    t.mday = 28;
    t.hour = 23;
    t.min  = 59;
    t.sec  = 58;

    test_rtc(t);
    sleep(1);    
}

/*month  0~11  year 2010~1025 */
void main()
{

    test_rtc_all();
    //alrm.enabled   = 1;
    /*set_alarm();
    sleep(2);
    get_alarm();*/
}