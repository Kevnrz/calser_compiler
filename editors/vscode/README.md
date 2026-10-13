# Calser para VS Code

Extensión local que registra `.cls` como **Calser** `.y` como **Yacc/Bison**
y `.l` como **Lex/Flex**.
Incluye resaltado, comentarios, parejas de corchetes/paréntesis y C embebido
para las acciones de Bison y Flex. No requiere dependencias ni ejecuta código.

## Instalar

Desde la raíz del repositorio:

```sh
python3 editors/vscode/package.py
code --install-extension editors/vscode/calser-language-0.3.0.vsix --force
```

En VS Code también puedes usar **Extensions: Install from VSIX...**.
Después ejecuta **Developer: Reload Window**. Al abrir un `.cls`, la barra
inferior debe indicar **Calser**; un `.y`, **Yacc/Bison**; y un `.l`, **Lex/Flex**.
La instalación registra estas extensiones en todos los proyectos.

### Si `.cls` aparece como TeX o LaTeX class

VS Code y LaTeX Workshop también registran `.cls` para clases de LaTeX.
La asociación explícita del proyecto en `.vscode/settings.json` da prioridad
a Calser al abrir este repositorio como carpeta:

```json
{
  "files.associations": {
    "*.cls": "calser"
  }
}
```

La carpeta `.vscode` está ignorada por Git en este repositorio; este ajuste
es local. Si instalas la extensión en otra máquina, añade esa asociación
al proyecto si hay conflicto con LaTeX.

Para desarrollar, abre `editors/vscode` como carpeta y ejecuta VS Code con
`code --extensionDevelopmentPath="$PWD/editors/vscode"` desde la raíz.

Esta versión no incorpora diagnósticos ni IntelliSense.
Los colores dependen del tema activo. Las reglas léxicas de Calser se basan
en `grammar/lexer.l`; el resaltado no valida la sintaxis del compilador.

## Icono de los programas Calser

La extensión incluye `icons/calser.png`, una copia de `assets/branding/calser-file-icon.png`, como icono del lenguaje y de la extensión.

Para que se muestre de forma consistente en `.cls`, ejecuta **Preferences:
File Icon Theme** y selecciona **Calser (Seti)**. Conserva los iconos de Seti
para los demás archivos; los programas Calser muestran el hexágono ámbar.
También puedes activarlo solamente en el proyecto con:

```json
{
  "files.associations": { "*.cls": "calser" },
  "workbench.iconTheme": "calser-seti"
}
```

El icono del lenguaje también puede aparecer con otros temas de iconos;
si el tema asigna su propio icono a `.cls`, puede tener prioridad.
El tema de colores del código se configura por separado.

Al compartir `editors/vscode`, el icono y el tema van incluidos. Cada
integrante debe generar e instalar el `.vsix` actualizado y seleccionar
**Calser (Seti)**, o añadir el ajuste del proyecto mostrado arriba.

Los iconos y la fuente Seti mantienen sus avisos en `ThirdPartyNotices.txt`.

## Lex/Flex

Los `.l` se registran como Lex/Flex. El resaltado distingue directivas como
`%option` y `%x`, separadores `%%`, estados `<COMMENT>`, `<<EOF>>`, cadenas,
clases de caracteres, escapes, cuantificadores y referencias `{NOMBRE}`.
Los bloques `%{ … %}`, las acciones (con o sin llaves) y el código final
usan la gramática de C de VS Code. Incluye comentarios y cierre de parejas.
