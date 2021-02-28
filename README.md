# SSE NIF Optimizer
Optimizes LE/SE NIF files and fixes various issues. Can also scan textures for compatibility.

### Build Requirements
- Microsoft Visual C++ 2017 (v141) or later
- wxWidgets 3.1.3 or later

### Building
Currently, only statically linking wxWidgets is supported by the build configuration.

- Building wxWidgets
  - Clone "wxWidgets" into your projects directory (parallel to the "SSE-NIF-Optimizer" clone)
  - Open up "wxWidgets\build\msw\wx_vc15.sln" or later in Visual Studio.
  - Set /MTd for the Debug, both Win32 and x64, configurations of all projects in the solution.
  - Set /MT for the Release, both Win32 and x64, configurations of all projects in the solution.
  - Build the Debug and Release configurations of the solution.
- Open up the SSE NIF Optimizer solution in Visual Studio
- Tested with MSVC++ 14.x (VS 2017) or higher

### Libraries used
- [wxWidgets](https://github.com/wxWidgets/wxWidgets) - GUI framework
- [nifly](https://github.com/ousnius/nifly) - C++ NIF library
  - half - IEEE 754-based half-precision floating point library
  - Miniball

### Credits
This tool would not have been possible without the help of:
- [jonwd7](https://github.com/jonwd7)
- [NifTools team](https://www.niftools.org/)
