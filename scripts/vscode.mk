SHELL := /bin/zsh

VSC_LAUN = $(NEMU_HOME)/.vscode/launch.json
define mod_laun
	jq ".configurations[0].$(1) $(2)= $(3)" $(VSC_LAUN) > $(VSC_LAUN).tmp && mv $(VSC_LAUN).tmp $(VSC_LAUN);
endef
define add_arg
	$(call mod_laun,args,+,[$(1)])
endef

vscode-debug: run-env
	$(call git_commit, "debug NEMU in VSCode")
	@args=($(ARGS)); \
	$(call mod_laun,program,,\"$(BINARY)\") \
	$(call mod_laun,args,,[]) \
	for arg in $$args; do \
		$(call add_arg, \"$$arg\") \
	done;
ifneq ($(IMG),)
	@$(call add_arg, \"$(IMG)\")
endif