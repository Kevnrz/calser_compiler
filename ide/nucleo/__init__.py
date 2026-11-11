"""Contratos productivos y material LL(1) de consulta, todos sin Qt.

asistencia contiene modelos/validación de ediciones consumidos por los
adaptadores C y la interfaz. No importa el lexer ni construye la tabla LL(1).
lexico, gramatica, tabla y parser se conservan para estudio, API de consulta y
pruebas diferenciales; M1–M5 ya no los cargan para analizar documentos del IDE.
No hay fallback al LL(1) si falta ccom. Importar este paquete no inicia nada.
Ver ide/nucleo/README.md y ide/docs/revision_m6.md para el mapa vigente.
"""
