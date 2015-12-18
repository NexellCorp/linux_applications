
========================================================================================================	
TINY ALSA (Lunux)

	SOURCE :
		https://github.com/tinyalsa

	- Not support resample
========================================================================================================


1. BUILD

	------------------------------------------------------------------------------------------------
	Makefile : 

	CROSS_COMPILE = arm-cortex_a9-linux-gnueabi-

	NOTE>  Static build: Refer to "Makefile.static"
	
		CFLAGS = -static .....
		......

			$(CROSS_COMPILE)gcc -static tinyplay.o
			......
			$(CROSS_COMPILE)gcc -static tinycp.o
			......
			$(CROSS_COMPILE)gcc -static tinymix.o
			......


		tinypcminfo: $(LIB) tinypcminfo.o
			$(CROSS_COMPILE)gcc -static tinypcminfo.o mixer.o pcm.o -o tinypcminfo
			......

		$(LIB): $(OBJECTS)
			$(CROSS_COMPILE)ar  -crv $(LIB_NAME).a $(OBJECTS)
	------------------------------------------------------------------------------------------------
		

	#> make
	------------------------------------------------------------------------------------------------
	EXE

		- tinyplay	: player
		- tinycap	: capture
		- tinymix	: mixer	
		- tinypcminfo	: hw info

	Library
	
		- libtinyalsa.so
			: tinyalsa library (mixer.o pcm.o)	
	------------------------------------------------------------------------------------------------


2. tinypcminfo	

	- Read sound card information
	------------------------------------------------------------------------------------------------
	#> tinypcminfo	-D 0 -p 0

	Info for card 0, device 0:

	PCM out:
	- "/dev/snd/pcmC0D0p"
        	Rate		:   min=48000Hz     max=48000Hz
	    	Channels	:   min=2           max=2
 		Sample 	bits	:   min=16          max=16
 		Period size	:   min=256         max=8192
		Period count	:   min=2           max=64

	PCM in:
	- "/dev/snd/pcmC0D0c"
        	Rate		:   min=48000Hz     max=48000Hz
    		Channels	:   min=2           max=2
 		Sample bits	:   min=16          max=16
 		Period size	:   min=256         max=8192
		Period count	:   min=2           max=64
	------------------------------------------------------------------------------------------------



3. tinymix
	
	- Read hw control info and config
	------------------------------------------------------------------------------------------------
	#> tinymix  or tinymix -D 0 

		ctl     type    num     name              	value
		0       ENUM    1       MIC1 Mode Control       Differential
		1       INT     1       MIC1 Boost              1
		.....

	- "/dev/snd/controlCx"


	Get control

		#> tinymix  1
		: Show "MIC1 Boost" status

			MIC1 Boost: 1 (range 0->8)

	Set control

		#> tinymix  1	1
		: Set "MIC1 Boost" value
	------------------------------------------------------------------------------------------------


4. tinyplay

	- wav player
	------------------------------------------------------------------------------------------------
	#> ./tinyplay ../sample.wav -D 0 -d 0 -c 2 -r 48000 -p 1024 -n 16

		- "/dev/snd/pcmC0D0p"

		-n	: periods
		-p	: period size (period buffer size = peroid size * channel * 2(16bit))
	------------------------------------------------------------------------------------------------
	

5. tinycap
	------------------------------------------------------------------------------------------------
	#> tinycap /system/test.wav -D 0 -d 0 -c 2 -r 48000 -b 16 -p 1024 -n 16
	------------------------------------------------------------------------------------------------






















