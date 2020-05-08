default_project = gnarl

projects = $(default_project) $(filter-out $(default_project),$(notdir $(wildcard src/*)))

$(projects):
	@mk/make-project.sh $@

clean:
	rm -rf project

%:
	@echo Please specify one of these projects: $(projects)
