# UnifiedEmu

Una sola app para Windows que reúne tus juegos de **Nintendo 64, GameCube, Wii y Wii U**
en una biblioteca y los abre con el emulador adecuado:

| Consola | Emulador | Licencia |
|---|---|---|
| Nintendo 64 | [Project64](https://github.com/project64/project64) | GPLv2 |
| GameCube y Wii | [Dolphin](https://github.com/dolphin-emu/dolphin) | GPLv2+ |
| Wii U | [Cemu](https://github.com/cemu-project/Cemu) | MPL-2.0 |

Los tres emuladores están incluidos como submódulos de git en `emulators/` y se compilan
desde su código fuente. El frontend (`frontend/`) está escrito en C++ con Qt 6.

> UnifiedEmu no incluye juegos, BIOS, firmware ni claves. Usa solo copias de tus propios
> juegos y consolas.

## Descargar

Descarga el ZIP de la última versión en
[Releases](https://github.com/aren4sjos3-lab/unified-emu/releases) y sigue la
[guía de instalación](docs/INSTALAR.md).

Cada push a `main` lanza el flujo **Compilar para Windows** en GitHub Actions; el paquete de
cada compilación está en la pestaña **Actions** → la ejecución → artefacto
**UnifiedEmu-windows**.

Para publicar una versión nueva, cambia el número en `project(UnifiedEmu VERSION x.y.z)`
de `frontend/CMakeLists.txt` y súbelo a `main`: la CI crea el tag `vx.y.z` y la versión en
Releases con el ZIP.

### Requisitos

- Windows 10 u 11 de 64 bits.
- [Visual C++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)
  x64 **y** x86 (Project64 es de 32 bits).
- Tarjeta gráfica con Direct3D 11.1 / Vulkan (Cemu necesita Vulkan u OpenGL 4.5).

## Uso

1. Descomprime la carpeta `UnifiedEmu` donde quieras (es portable: toda la configuración se
   guarda dentro de ella).
2. Abre `UnifiedEmu.exe` y añade las carpetas donde tienes tus juegos.
3. Haz doble clic en un juego (o selecciónalo y pulsa Intro) para jugar.

Los mandos, gráficos y audio de cada consola se configuran desde el menú **Emuladores**,
que abre la interfaz de cada emulador sin juego.

**Wii U:** Cemu necesita las claves de tu consola (`keys.txt`) o sus archivos online para
descifrar los juegos. Ábrelo desde **Emuladores → Abrir Cemu** y sigue su asistente inicial.

### Formatos reconocidos

La consola se detecta leyendo la cabecera de cada archivo, no solo por la extensión.

- **Nintendo 64:** `.z64`, `.n64`, `.v64`, `.u64` (los tres órdenes de bytes).
- **GameCube / Wii:** `.iso`, `.gcm`, `.rvz`, `.wia`, `.gcz`, `.wbfs`, `.ciso`, `.tgc`,
  `.wad`, `.dol`, `.elf`.
- **Wii U:** `.wua`, `.wud`, `.wux`, `.rpx` y carpetas de títulos extraídos
  (`code/*.rpx` + `meta/meta.xml`).

## Estructura

```
emulators/
  project64/   submódulo → Project64
  dolphin/     submódulo → Dolphin
  cemu/        submódulo → Cemu
frontend/
  src/core/    detección de juegos, configuración y lanzamiento
  src/ui/      ventana principal, lista de juegos y ajustes
  tests/       pruebas del detector
.github/workflows/build-windows.yml
```

Paquete generado:

```
UnifiedEmu/
  UnifiedEmu.exe
  UnifiedEmu.ini        (se crea al guardar la configuración)
  emulators/
    Project64/Project64.exe
    Dolphin/Dolphin.exe
    Cemu/Cemu.exe
```

## Modificar los emuladores (forks)

Los submódulos apuntan a los repositorios oficiales. Para cambiar el código de un emulador:

1. Haz fork en GitHub (por ejemplo de `dolphin-emu/dolphin`).
2. Cambia la URL en `.gitmodules` por la de tu fork y ejecuta `git submodule sync`.
3. Haz tus cambios dentro de `emulators/<emulador>`, súbelos a tu fork y haz commit del
   nuevo puntero del submódulo en este repositorio.

Para actualizar un emulador a su última versión oficial:

```bash
git submodule update --remote emulators/dolphin
git commit -am "Actualizar Dolphin"
```

La CI guarda en caché cada emulador compilado según el commit de su submódulo, así que solo se
recompila el que haya cambiado.

## Compilar localmente

Necesitas Visual Studio 2022 (con C++ de escritorio y ATL), CMake ≥ 3.25, Ninja y Qt 6.8 para
MSVC. Los comandos exactos de cada parte están en
[`.github/workflows/build-windows.yml`](.github/workflows/build-windows.yml). Para el frontend:

```bash
cmake -S frontend -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

El detector de juegos no depende de Qt y sus pruebas se pueden compilar en cualquier sistema
con `-DUNIFIEDEMU_BUILD_GUI=OFF`.

## Licencia

UnifiedEmu se distribuye bajo la GPLv2 (ver `LICENSE`). Cada emulador conserva su propia
licencia, incluida en su carpeta dentro del paquete.
