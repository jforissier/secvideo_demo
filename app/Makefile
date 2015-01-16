.PHONY: all host ta clean clean-host clean-ta

all: host ta

clean: clean-host clean-ta

host:
	$(MAKE) -C host

ta:
	$(MAKE) -C ta

clean-host:
	$(MAKE) -C host clean

clean-ta:
	$(MAKE) -C ta clean

distclean:
	$(MAKE) -C host distclean
