#!/usr/bin/env bash
# Publica las líneas de error de un log de compilación como anotaciones del job,
# para poder ver la causa de un fallo sin descargar el log completo.
set -u
log="$1"
title="${2:-Compilación}"
[ -f "$log" ] || { echo "::error title=$title::No existe el log $log"; exit 0; }

escape() { sed -e 's/%/%25/g' -e 's/\r//g' | sed -e ':a;N;$!ba;s/\n/%0A/g'; }

errors=$(grep -iE "error|fatal|failed|not found|could not" "$log" | grep -viE "^\s*-- |warning" | head -n 40 | cut -c1-400)
if [ -n "$errors" ]; then
  echo "::error title=$title (errores)::$(printf '%s' "$errors" | escape)"
fi
echo "::error title=$title (final del log)::$(tail -n 40 "$log" | cut -c1-400 | escape)"
