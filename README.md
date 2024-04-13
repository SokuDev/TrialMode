# TrialMode
Adds a combo trial mode to Touhou 12.3

# Build
Requires CMake, git and the VisualStudio compiler (MSVC).
Both git and cmake needs to be in the PATH environment variable.

All the following commands are to be run inside the visual studio 32bits compiler
command prompt (called `x86 Native Tools Command Prompt for VS 20XX` in the start menu), unless stated otherwise.

## Initialization
First go inside the folder you want the repository to be in.
In this example it will be C:\Users\PinkySmile\SokuProjects but remember to replace this
with the path for your machine. If you don't want to type the full path, you can drag and
drop the folder onto the console.

`cd C:\Users\PinkySmile\SokuProjects`

Now let's download the repository and initialize it for the first time
```
git clone https://github.com/SokuDev/TrialMode
cd TrialMode
git submodule init
git submodule update
mkdir build
cd build
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
```
Note that if you want to build in Release, you should replace `-DCMAKE_BUILD_TYPE=Debug` with `-DCMAKE_BUILD_TYPE=Release`.

## Compiling
Now, to build the mod, go to the build directory (if you did the previous step you already are)
`cd C:\Users\PinkySmile\SokuProjects\TrialMode\build` and invoke the compiler by running `cmake --build . --target TrialMode`. If you change the name of the mod (in the add_library statement in CMakeLists.txt), you will need to replace 'TrialMode' by the name of your mod in the previous command.

You should find the resulting TrialMode.dll mod inside the build folder that can be to SWRSToys.ini.
In my case, I would add this line to it `TrialMode=C:/Users/PinkySmile/SokuProjects/TrialMode/build/TrialMode.dll`.
