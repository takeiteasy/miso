# miso

Mini isometric game engine **WIP**

## TODO 

- [ ] Implement height
- [ ] Add screen <-> world position functions
- [ ] Finalize API + ~~add namespace~~
- [ ] Example: Integrate [.tmx loader](https://github.com/baylej/tmx)
- [ ] Custom map loader (Optional) [in-progress]
- [ ] Lua bindings
- [ ] Documentation

## Gallery

Nothing to see yet

## Dependencies

- [floooh/sokol](https://github.com/floooh/sokol) (zlib/libpng)
    - sokol_gfx.h
    - sokol_app.h
    - sokol_glue.h
    - sokol_nuklear.h
- [Immediate-Mode-UI/Nuklear](https://github.com/Immediate-Mode-UI/Nuklear) (MIT/Public Domain)
    - nuklear.h
- [nothings/stb](https://github.com/nothings/stb/tree/master) (MIT/Public Domain)
    - stb_image.h
- [phoboslab/qoi](https://github.com/phoboslab/qoi) (MIT)
    - qoi.h
- [tsoding/jim](https://github.com/tsoding/jim) (MIT)
    - jim.h
- [esr/microjson](https://gitlab.com/esr/microjson/) (BSD-2-clause)
    - mjson.h
- [AndrewBelt/osdialog](https://github.com/AndrewBelt/osdialog) (CC0)
    - osdialog.h
    
## LICENSE
```
The MIT License (MIT)

Copyright (c) 2022 George Watson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
