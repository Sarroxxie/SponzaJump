# Sponza Jump

This small game was created for a course at the KIT (Karlsruhe Institute of Technology) over a period of 12 weeks. Since the course is now over, this project is not in active development anymore.

![](res/docs/images/game_preview.png)

## Setup

### Cloning
This repository contains [submodules](https://git-scm.com/book/de/v2/Git-Tools-Submodule) for some of its libraries. Cloning it does not automatically clone its submodules. 

Either clone it with the ```"--recurse-submodules"``` flag or, if the project is already cloned, run these two commands:
```
git submodule init
git submodule update
```

### Building
The whole build process is being executed when calling **cmake** in the root directory. We recommend to create a **/build** directory inside the root directory and to call ```"cmake .."``` from inside the build directory. This way all the built files will be written into **/build** and are also already being excluded from the repository via .gitignore.

### VulkanSDK
CMake will automatically try to grab the VulkanSDK path via ```"find_package(Vulkan REQUIRED)"```. For this to work, the environment variable **VULKAN_SDK** has to point to a VulkanSDK installation.  
This project uses the VulkanSDK ([official download page](https://vulkan.lunarg.com/sdk/home)) for shader compilation. The version the project got developed on is the **VulkanSDK 1.3.221.0**, so while other versions might work, it is not guaranteed.

### Working Directory
The working directory of the project needs to be set to the project root directory. This is usually done inside the project settings in the IDE of your choice.