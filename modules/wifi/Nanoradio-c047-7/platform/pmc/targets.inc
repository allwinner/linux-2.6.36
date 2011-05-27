
pmcapi: obj nrxapi
	(cd userspace/libpmc; \
	autoreconf -fi; \
	./configure --host=$(HOST) \
	CFLAGS="-I$(KERNEL_DIR)/include \
	$(addprefix -I$(HOSTDRIVER_DIR)/, $(NANO_IF_INC) \
	 $(WIFI_ENGINE_INC) )"; \
	make;)
	cp userspace/libpmc/autotest/pmcpriv $(OBJ_DIR)
