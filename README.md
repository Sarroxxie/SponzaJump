# Graphics and Game Development Praktikum

Project for the graphics and game development praktikum during SS23 of Josias Distler (uiqvb) and Jan-Luca Gutsch (uhivi).

## Setup

### Working Directory

The working directory needs to be set to the project root directory.

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