default_project = pickle

projects = $(default_project) $(filter-out $(default_project),$(notdir $(wildcard src/*)))

$(projects):
	@mk/make-project.sh $@

clean:
	rm -fr project

%:
	@echo Please specify one of these projects: $(projects)
