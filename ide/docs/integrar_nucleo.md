# Conexión vigente del backend de CALSIDE

Actualizado en M6, 2026-11-11. La entrega y el merge originales ya ocurrieron; no hay que repetirlos. La arquitectura actual reutiliza ccom para las decisiones del lenguaje. La excepción acordada permite mantener LL(1) y simulador en Python; el LL(1) se conserva para consulta/pruebas, fuera del recorrido gráfico.

## Qué llama la interfaz

| Acción | Servicio real | Adaptación Python |
| --- | --- | --- |
| Léxico y colores | Flex, `--tokens-json` | `adaptador_nucleo.py`, `interfaz/analisis.py`. |
| Análisis automático | Flex/Bison/semántico, `--check-json` | `ccom.py`, `interfaz/comprobacion.py`. |
| Sugerencias | Bison/LAC y Flex, `--suggest-json` | `sugerencias_ccom.py`. |
| Reparaciones | Candidatos y reanálisis C, `--repair-json` | `reparaciones_ccom.py`, panel/aplicador. |
| Comprobación de regla automática | Parser C, `--syntax-json` | `politica_asistencia.py`: preferencias y contexto. |
| Clones | Flex y comparación C, `--clones-json` | `clones.py`: varias instantáneas y posiciones. |

Los adaptadores comparten `ccom.informe_json` para la cabecera; después validan su contenido. El transporte M2–M5 está en `servicio_ccom.py` dentro de trabajadores; el análisis automático M1 conserva QProcess. Los textos se envían como copias privadas sin guardar buffers. La ventana aporta el ccom configurado y descarta revisiones antiguas.

Los únicos imports productivos desde `ide.nucleo` son modelos/validadores de `asistencia.py`. No conectar de nuevo `lexico.analizar` al resaltado ni `parser.contexto_en` a las sugerencias gráficas. Para consultar el parser LL(1) directamente, ver [su API](parser_predictivo.md) y [su papel actual](../nucleo/README.md).

## Posiciones y memoria

C: bytes UTF-8 `[start,end)`. Contratos de ediciones: caracteres Python. Qt: unidades UTF-16. Los adaptadores convierten posiciones; el aplicador verifica el contexto y agrupa una propuesta en un Deshacer. Los diagnósticos visuales no sustituyen el rango real del token ni la posición de una edición.

Los servicios nativos atienden una consulta por proceso, liberan sus buffers/AST y no tocan archivos de usuario. Cancelar mata/recoge el hijo y elimina la copia; el IDE conserva documentos e historial. El LL(1) Python no se carga si ccom falla: se informa la ausencia/incompatibilidad.

## Verificación

```sh
make
make ide
make check
make check-ide
make check-recorrido-ide
```

[Guía unificada](guia_sistema_calside.md), [decisiones y limpieza M6](revision_m6.md), [etapas](plan_migracion_logica_c.md). La recuperación de varios errores sintácticos es M7; no la completa el hecho de transportar una lista JSON.
