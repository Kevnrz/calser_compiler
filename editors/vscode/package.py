"""Empaqueta esta extensión declarativa sin dependencias externas."""
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from xml.sax.saxutils import escape

root = Path(__file__).resolve().parent
manifest = json.loads((root / 'package.json').read_text())
output = root / f"{manifest['name']}-{manifest['version']}.vsix"
vsix_manifest = f'''<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011">
  <Metadata>
    <Identity Language="en-US" Id="{escape(manifest['name'])}" Version="{escape(manifest['version'])}" Publisher="{escape(manifest['publisher'])}" />
    <DisplayName>{escape(manifest['displayName'])}</DisplayName>
    <Description xml:space="preserve">{escape(manifest['description'])}</Description>
    <Tags>calser,yacc,bison,lex,flex</Tags><Categories>Programming Languages</Categories>
    <Properties><Property Id="Microsoft.VisualStudio.Code.Engine" Value="{escape(manifest['engines']['vscode'])}" /></Properties>
  </Metadata>
  <Installation><InstallationTarget Id="Microsoft.VisualStudio.Code" /></Installation>
  <Dependencies />
  <Assets><Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true" /></Assets>
</PackageManifest>'''
content_types = '''<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="json" ContentType="application/json" />
  <Default Extension="md" ContentType="text/markdown" />
  <Default Extension="png" ContentType="image/png" />
  <Default Extension="woff" ContentType="font/woff" />
  <Default Extension="txt" ContentType="text/plain" />
  <Default Extension="vsixmanifest" ContentType="text/xml" />
</Types>'''
files = ['package.json', 'README.md']
files += sorted({language['configuration'].removeprefix('./') for language in manifest['contributes']['languages'] if 'configuration' in language})
files += [str(p.relative_to(root)) for p in sorted((root / 'syntaxes').glob('*.json'))]
files += [str(p.relative_to(root)) for p in sorted((root / 'icons').glob('*')) if p.is_file()]
files += ['ThirdPartyNotices.txt']
with ZipFile(output, 'w', ZIP_DEFLATED) as archive:
    archive.writestr('extension.vsixmanifest', vsix_manifest)
    archive.writestr('[Content_Types].xml', content_types)
    for filename in files:
        archive.write(root / filename, f'extension/{filename}')
print(output)
