default_project = gnarl

projects = $(default_project) $(filter-out $(default_project),$(notdir $(wildcard apps/*)))

$(projects):
	@mk/make-project.sh $@

clean:
	rm -fr project

%:
	@echo Please specify one of these projects: $(projects)
