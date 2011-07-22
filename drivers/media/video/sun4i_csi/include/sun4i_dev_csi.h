#ifndef __DEV_CSI_H__
#define __DEV_CSI_H__


/*
 * ioctl to proccess sub device
 */
typedef enum tag_CSI_SUBDEV_CMD
{
	CSI_SUBDEV_CMD_GET_INFO = 0x01,
	CSI_SUBDEV_CMD_SET_INFO = 0x02,
}__csi_subdev_cmd_t;

#endif
