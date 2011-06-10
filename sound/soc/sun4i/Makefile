obj-$(CONFIG_SND_SUN4I_SOC_CODEC) += sun4i-codec.o

# SUN4I Audio Card Support
obj-$(CONFIG_SND_SUN4I_SOC_I2S) += sun4i_iis.o
sun4i_iis-objs := sun4i-anx7150.o sun4i-i2s.o anx7150.o sun4i-pcm.o
 
# hdmi audio out with i2s interface
#obj-$(CONFIG_SND_SUN4I_SOC_HDMI) += sun4i-anx7150.o sun4i-i2s.o
#obj-$(CONFIG_SND_SUN4I_SOC_HDMI) += anx7150.o

obj-$(CONFIG_SND_SUN4I_SOC_SPDIF) += sun4ispdif.o
sun4ispdif-objs := sun4i_spdma.o sun4i_spdif.o anx7150sp.o sun4i_anx7150sp.o 

# AC97 audio out
#obj-$(CONFIG_SND_AW16XX_SOC_AC97) +=

# pcm out with on chip codec
