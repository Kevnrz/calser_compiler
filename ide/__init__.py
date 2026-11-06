"""Paquete Python de CALSIDE, el IDE de escritorio del lenguaje Calser.

Organización principal:
    main: punto de entrada que crea la aplicación y abre la ventana.
    interfaz: widgets PySide6 y coordinación de sus acciones y trabajadores.
    nucleo: lexer, gramática, parser LL(1) y contratos sin dependencia de Qt.
    Otros módulos de esta carpeta: servicios del IDE, como archivos, búsqueda,
    diagnósticos, clones, preferencias y validación de ediciones.

Este archivo identifica el paquete sin iniciar servicios ni importar la ventana.
Así, importar un módulo de lógica o ejecutar sus pruebas no abre la aplicación.
Para iniciar CALSIDE se utiliza ``python3 -m ide.main``.
"""
