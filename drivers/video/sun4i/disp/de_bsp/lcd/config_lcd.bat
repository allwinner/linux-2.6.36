@echo off
	goto START

:START
    @echo *********************************************
    @echo *   select board                            *
    @echo *********************************************
    @echo  0: fpga
    @echo *********************************************
    
    set /p SEL=Please Select:
    if %SEL%==0     goto LCD0
    goto ERROR

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:LCD0
	copy lcd_bak\fpga_0.c lcd0_panel_cfg.c
	copy lcd_bak\fpga_1.c lcd1_panel_cfg.c
    goto conti
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    
:ERROR
	@echo error selection
	goto conti
    
:conti
		del lcd0_panel_cfg.o
		del lcd1_panel_cfg.o
    @echo *********************************************
    pause

