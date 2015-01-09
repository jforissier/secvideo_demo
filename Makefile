LINUX_URL = https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.18.tar.xz
LINUX_TARBALL = $(call filename,$(LINUX_URL))
LINUX_DIR = $(LINUX_TARBALL:.tar.xz=)

EDK2_URL = https://github.com/tianocore/edk2/archive/8c83d0c0b9bd102cd905c83b2644a543e9711815.tar.gz
EDK2_TARBALL = $(call filename,$(EDK2_URL))
EDK2_DIR = edk2-$(EDK2_TARBALL:.tar.gz=)

AARCH64_GCC_URL = http://releases.linaro.org/14.08/components/toolchain/binaries/gcc-linaro-aarch64-linux-gnu-4.9-2014.08_linux.tar.xz
AARCH64_GCC_TARBALL = $(call filename,$(AARCH64_GCC_URL))
AARCH64_GCC_DIR = $(AARCH64_GCC_TARBALL:.tar.xz=)

AARCH64_NONE_GCC_URL = http://releases.linaro.org/14.07/components/toolchain/binaries/gcc-linaro-aarch64-none-elf-4.9-2014.07_linux.tar.xz
AARCH64_NONE_GCC_TARBALL = $(call filename,$(AARCH64_NONE_GCC_URL))
AARCH64_NONE_GCC_DIR = $(AARCH64_NONE_GCC_TARBALL:.tar.xz=)

CURL = curl -L

filename = $(lastword $(subst /, ,$(1)))

ifeq ($(V),1)
  Q :=
  ECHO := @:
else
  Q := @
  ECHO := @echo
endif

all: linux aarch64-gcc aarch64-none-gcc edk2
    

linux: $(LINUX_TARBALL)
	$(ECHO) '  TAR    $@'
	$(Q)rm -rf $(LINUX_DIR)
	$(Q)tar xf $(LINUX_TARBALL)
	$(Q)rm -rf $@
	$(Q)mv $(LINUX_DIR) $@
	$(Q)touch $@

$(LINUX_TARBALL):
	$(ECHO) '  CURL   $@'
	$(Q)$(CURL) $(LINUX_URL) -o $@


edk2: $(EDK2_TARBALL)
	$(ECHO) '  TAR    $@'
	$(Q)rm -rf $(EDK2_DIR)
	$(Q)tar xf $(EDK2_TARBALL)
	$(Q)rm -rf $@
	$(Q)mv $(EDK2_DIR) $@
	$(Q)touch $@

$(EDK2_TARBALL):
	$(ECHO) '  CURL   $@'
	$(Q)$(CURL) $(EDK2_URL) -o $@


aarch64-gcc: toolchains/$(AARCH64_GCC_DIR)

toolchains/$(AARCH64_GCC_DIR): toolchains/$(AARCH64_GCC_TARBALL)
	$(ECHO) '  TAR    $@'
	$(Q)rm -rf toolchains/$(AARCH64_GCC_DIR)
	$(Q)cd toolchains && tar xf $(AARCH64_GCC_TARBALL)
	$(Q)touch $@

toolchains/$(AARCH64_GCC_TARBALL):
	$(ECHO) '  CURL   $@'
	$(Q)$(CURL) $(AARCH64_GCC_URL) -o $@


aarch64-none-gcc: toolchains/$(AARCH64_NONE_GCC_DIR)

toolchains/$(AARCH64_NONE_GCC_DIR): toolchains/$(AARCH64_NONE_GCC_TARBALL)
	$(ECHO) '  TAR    $@'
	$(Q)rm -rf toolchains/$(AARCH64_NONE_GCC_DIR)
	$(Q)cd toolchains && tar xf $(AARCH64_NONE_GCC_TARBALL)
	$(Q)touch $@

toolchains/$(AARCH64_NONE_GCC_TARBALL):
	$(ECHO) '  CURL   $@'
	$(Q)$(CURL) $(AARCH64_NONE_GCC_URL) -o $@


clean:
	$(ECHO) '  CLEAN  .'

cleaner: clean
	$(ECHO) '  CLEAN+ .'
	$(Q)rm -rf $(LINUX_DIR) linux
	$(Q)rm -rf $(EDK2_DIR) edk2
	$(Q)rm -rf toolchains/$(AARCH64_GCC_DIR)
	$(Q)rm -rf toolchains/$(AARCH64_NONE_GCC_DIR)

# Also remove downloaded files
distclean: cleaner
	$(ECHO) '  DISTCL .'
	$(Q)rm -f $(LINUX_TARBALL)
	$(Q)rm -f $(EDK2_TARBALL)
	$(Q)rm -f toolchains/$(AARCH64_GCC_TARBALL)
	$(Q)rm -f toolchains/$(AARCH64_NONE_GCC_DIR)

