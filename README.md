# Graphics and Game Development Praktikum

This small game was created for a course at the KIT (Karlsruher Institut für Technologie). Since the course is now over, this project is not in active development anymore.

## Setup

### Working Directory

The working directory of the project needs to be set to the project root directory. This is usually done inside the project settings in the IDE of your choice.

### Submodules

This repository contains [submodules](https://git-scm.com/book/de/v2/Git-Tools-Submodule) for some of its libraries. Cloning it does not automatically clone its submodules. 

Either clone it with the `--recurse-submodules` flag:
```
git clone git@git.scc.kit.edu:uhivi/graphicspraktikum.git --recurse-submodules
```

or, if the project is already cloned, run these two commands:
```
git submodule init
git submodule update
```