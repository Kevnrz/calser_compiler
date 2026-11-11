# Contexto vigente del motor de reparaciones

Actualizado en M6, 2026-11-11. La entrega original Python se usó como referencia de comportamiento; el motor productivo ya está adaptado a **C** y conectado al IDE. El reparto de noviembre 5 que exigía Python puro queda sustituido por esta arquitectura. El LL(1) se conserva para consulta/estudio, no como dependencia de este motor.

## Dónde trabajar

- Backend: `src/repairs.c, src/syntax_service.c y sus cabeceras`.
- Entrada nativa: `ccom --repair-json y --syntax-json`; stdout JSON v1 y stderr para auxiliares.
- Adaptador: `ide/reparaciones_ccom.py`. Valida datos y convierte coordenadas; no interpreta Calser.
- Modelos compartidos: `ide/nucleo/asistencia.py`, sin importar los analizadores Python.
- Contrato nativo detallado: [ccom_repairs_json.md](../../docs/ccom_repairs_json.md).
- Contrato Python/UI: [contrato_asistencia.md](contrato_asistencia.md).

## Frontera de responsabilidades

C obtiene contexto/expectativas del lexer/parser real y decide las propuestas. Python conserva archivos/pestañas, instantáneas, permisos, revisión vigente, interfaz y aplicación. No duplicar reglas de lenguaje en un proveedor Python ni crear otro modelo incompatible. El coordinador captura root/ccom configurado y usa el transporte cancelable compartido; no leer obligatoriamente la fuente original ni guardar buffers.

C usa bytes UTF-8 `[start,end)`, los modelos usan caracteres Python y Qt usa UTF-16. No convertir un comentario humano en precondición ni confundir el rango del mensaje visual con el de la edición. Los buffers/AST temporales pertenecen al servicio C y se liberan antes de retornar. Flex/Bison y observadores son seriales dentro del proceso; la concurrencia del IDE usa procesos separados.

## Comportamiento y límites

Límites nativos: 4 propuestas, 200000 bytes y 50000 reconocimientos. El hijo tiene un plazo de 2,5 s en el transporte y el coordinador gráfico un presupuesto total de 3 s. Cero opciones válidas, error previo que bloquea, límite y fallo de servicio son estados distintos. La cancelación se propaga al transporte y termina/recoge el hijo sin cambiar documentos.

Solo se insertan `;`, `.`, `]` y `)` esperados por LAC. Cada copia se reanaliza íntegramente; una mejora parcial es solo manual. Las precondiciones contienen texto original real y no vacío. No se inventan `E`, operandos ni reparaciones semánticas.

`automatic_safe` no es permiso: la política Python exige una única propuesta de la regla revisada INSERCION_PUNTO_COMA, habilitación/selección del usuario, contexto vigente y aceptación completa por C. Se verifican original/candidato con `--syntax-json`. No guardar, no encadenar y conservar Deshacer.

## Pruebas antes de cambiar el motor

`make check-repairs-json`, `make check`, `make check-ide` y `make check-recorrido-ide`. Los casos del lenguaje deben usar C real; los dobles solo aíslan coordinación, transporte o fallos de interfaz.

[Revisión M6 y orden de lectura](revision_m6.md). La sincronización para varios errores sintácticos fiables se implementará en M7.
