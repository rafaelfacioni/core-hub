.PHONY:gccall
gccall:
	(cd Applications/$(PROJECT) && $(MAKE) V=$(V) -f Makefile all)

.PHONY:clean_all
clean_all:
ifneq ("$(wildcard Build/)","")
	@$(RM) -r Build/
endif

.PHONY:clean-gccall
clean-gccall:
	(cd Applications/$(PROJECT) && $(MAKE) -f Makefile cleanall)

.PHONY:clean-gcc
clean-gcc:
	(cd Applications/$(PROJECT) && $(MAKE) -f Makefile clean)
.PHONY:size
size:
	(cd Applications/$(PROJECT) && $(MAKE) -f Makefile size)

.PHONY:jflash
jflash:
	(JLink -commanderscript Applications/$(PROJECT)/Launch/jflash)

