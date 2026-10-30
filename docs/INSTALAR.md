# Instalar y probar UnifiedEmu

UnifiedEmu reúne Project64 (Nintendo 64), Dolphin (GameCube y Wii) y Cemu (Wii U) en una sola
app para Windows. No incluye juegos, BIOS ni claves: usa copias de tus propios juegos y consolas.

## 1. Requisitos

- Windows 10 u 11 de **64 bits**.
- Los **Visual C++ Redistributable** de Microsoft, en sus dos versiones (Project64 es de 32 bits
  y Dolphin y Cemu de 64 bits):
  - x64: https://aka.ms/vs/17/release/vc_redist.x64.exe
  - x86: https://aka.ms/vs/17/release/vc_redist.x86.exe
- Drivers de la tarjeta gráfica actualizados (Cemu necesita Vulkan u OpenGL 4.5).

Instala los dos redistributables antes de seguir. Si ya los tienes, el instalador ofrece
"Reparar" o "Cerrar": cualquiera de las dos vale.

## 2. Descargar

1. Abre la página de versiones del proyecto:
   https://github.com/aren4sjos3-lab/unified-emu/releases
2. En la versión más reciente, descarga `UnifiedEmu-vX.Y.Z-windows.zip` (sección **Assets**).

## 3. Descomprimir

1. **Desbloquea el ZIP antes de extraerlo:** clic derecho en el ZIP → **Propiedades** → marca
   **Desbloquear** (abajo, junto a "Seguridad") → **Aceptar**. Así Windows no mostrará un aviso
   por cada programa de dentro.
2. Clic derecho en el ZIP → **Extraer todo…** y elige una carpeta donde tengas permiso de
   escritura, por ejemplo `C:\Juegos`. Se creará `C:\Juegos\UnifiedEmu`.

   No lo pongas en `C:\Archivos de programa`: la configuración se guarda dentro de la carpeta de
   UnifiedEmu y ahí Windows no deja escribir.

La carpeta queda así:

```
UnifiedEmu\
  UnifiedEmu.exe          ← la app que abres
  emulators\
    Project64\Project64.exe
    Dolphin\Dolphin.exe
    Cemu\Cemu.exe
```

## 4. Primer arranque

1. Abre `UnifiedEmu.exe`.
2. Si aparece **"Windows protegió su PC"** (SmartScreen), pulsa **Más información** →
   **Ejecutar de todas formas**. Sale porque el programa no está firmado digitalmente.
3. UnifiedEmu te pedirá las carpetas de juegos. Pulsa **Añadir carpeta…** y elige cada carpeta
   donde guardas juegos. Deja marcada **Buscar también en subcarpetas**.
4. Pulsa **Aceptar**. La lista se llena con los juegos encontrados y en la barra inferior verás
   cuántos hay de cada consola.

Puedes cambiar las carpetas y activar **Iniciar los juegos en pantalla completa** en
**Archivo → Configuración…**.

## 5. Configurar cada emulador (una sola vez)

Los mandos, gráficos y audio se configuran en la interfaz de cada emulador. Ábrelos desde el
menú **Emuladores** de UnifiedEmu. Si algún emulador aparece en inglés, los nombres en inglés de
los menús van entre paréntesis.

### Nintendo 64 — Project64

**Emuladores → Abrir Project64** → menú **Opciones → Configurar plugin de control…** (en inglés:
*Options → Configure Controller Plugin…*) y asigna los botones de tu mando. Cierra Project64 al
terminar.

### GameCube y Wii — Dolphin

**Emuladores → Abrir Dolphin** → botón **Mandos**:

- **GameCube:** en "Puertos de GameCube", puerto 1 → **Mando estándar** → **Configurar** y
  asigna los botones.
- **Wii:** en "Wiimotes", Wiimote 1 → **Wiimote emulado** → **Configurar**. Si tienes un Wiimote
  real y un adaptador Bluetooth, elige **Wiimote real**.

Cierra Dolphin al terminar.

### Wii U — Cemu

**Emuladores → Abrir Cemu**. La primera vez aparece un asistente:

1. Acepta la carpeta de datos (MLC) que propone: queda dentro de `emulators\Cemu\portable`.
2. La descarga de "graphic packs" es opcional (mejoras de resolución y correcciones).
3. Configura el mando en **Opciones → Configuración de entrada** (*Options → Input settings*). Elige **Wii U GamePad** como
   controlador emulado y asigna los botones.

**Juegos de Wii U y claves:**

- Los títulos extraídos (carpeta con `code`, `content` y `meta`) y los archivos `.wua` funcionan
  sin claves.
- Los discos `.wud` / `.wux` están cifrados: necesitan un `keys.txt` con las claves de **tu**
  consola, copiado en `emulators\Cemu\keys.txt`.

## 6. Probar

Haz esta prueba con un juego de cada consola que tengas:

1. Doble clic en el juego (o selecciónalo y pulsa **Jugar**).
2. Comprueba que se abre el emulador correcto, se ve la imagen, se oye el audio y el mando
   responde.
3. **Alt+Intro** cambia entre ventana y pantalla completa.
4. Para salir, cierra la ventana del juego; vuelves a UnifiedEmu. Con Dolphin también puedes
   pulsar **Esc** (para la emulación y se cierra).

Prueba también:

- El desplegable **Todas las consolas** filtra la lista por consola.
- La caja **Buscar** filtra por título o ID.
- Clic derecho en un juego → **Abrir carpeta del juego**.

## 7. Problemas frecuentes

| Problema | Solución |
|---|---|
| Falta `VCRUNTIME140.dll` o `MSVCP140.dll` | Instala los dos Visual C++ Redistributable del paso 1. |
| "No se encuentra Project64/Dolphin/Cemu en…" | Comprueba que la carpeta `emulators` está junto a `UnifiedEmu.exe` y que extrajiste el ZIP completo. |
| Un juego no aparece en la lista | Revisa que su carpeta esté añadida y que el formato sea uno de los soportados (ver README). Pulsa **Actualizar lista** (F5). |
| Cemu no arranca o da error de Vulkan | Actualiza los drivers de la tarjeta gráfica. |
| Un juego de Wii U `.wud`/`.wux` no carga | Falta `keys.txt` o no tiene la clave de ese disco. |
| Windows bloquea los programas | Repite el paso 3.1 (Desbloquear) con un ZIP recién descargado. |

## 8. Informar de un fallo

Abre un issue en https://github.com/aren4sjos3-lab/unified-emu/issues con:

- Qué hiciste y qué esperabas que pasara.
- Qué pasó en realidad (una captura ayuda mucho).
- Consola y formato del juego (por ejemplo "Wii, .rvz").
