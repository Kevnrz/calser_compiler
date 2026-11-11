"""M5: puente estricto, cancelación del hijo y motor nativo sin lexer Python."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
from ide import ccom
from ide.clones import Cancelado, LimiteAnalisis, detectar, empaquetar, interpretar, tokenizar
from ide.servicio_ccom import consultar, LimiteServicio
ROOT = Path(__file__).resolve().parents[2]
FUENTE = Path('/tmp/snapshot-clones.ccl')
TEXT = '# café 😀\r\nx=x+1;'
SOURCES = {Path('/tmp/a.cls'): TEXT, Path('/tmp/b.cls'): TEXT}


def informe():
    raw=TEXT.encode(); inicio=raw.index(b'x=')
    return {'schema_version':1,'operation':'clones','offset_unit':'utf8','source':str(FUENTE),
            'exit_code':0,'status':'ok','minimum':5,'explanation':'',
            'files':[{'id':i,'path':str(p),'bytes':len(raw)} for i,p in enumerate(SOURCES)],
            'pairs':[{'type':'I','tokens':6,'a':{'file':0,'start':inicio,'end':len(raw),'line_start':2,'line_end':2},
                      'b':{'file':1,'start':inicio,'end':len(raw),'line_start':2,'line_end':2}}]}


class ClonesNativos(unittest.TestCase):
    def test_no_usa_lexer_o_parser_python(self):
        with patch('ide.nucleo.lexico.analizar',side_effect=AssertionError('Sin lexer Python')), \
             patch('ide.nucleo.parser.analizar_sintaxis',side_effect=AssertionError('Sin LL1')):
            pares=detectar(SOURCES,5)
            tokens=tokenizar(TEXT)
        self.assertEqual(len(pares),1)
        self.assertEqual(len(tokens),6)
        self.assertEqual(pares[0].primero.inicio,TEXT.index('x='))
        self.assertEqual(pares[0].primero.linea_inicio,2)

    def test_metadatos_y_rangos_invalidos_rechazados(self):
        _,docs=empaquetar(SOURCES)
        original=informe()
        par=interpretar(json.dumps(original),FUENTE,0,docs,5)[0]
        self.assertEqual(TEXT[par.primero.inicio:par.primero.fin],'x=x+1;')
        casos=[]
        for campo,valor in [('schema_version',True),('operation','lex'),('offset_unit','chars'),
                            ('source','otra'),('minimum',6),('exit_code',True),('status','unknown')]:
            d=copy.deepcopy(original);d[campo]=valor;casos.append(d)
        for campo,valor in [('file',True),('file',2),('start',-1),('end',1000),('start',9),
                            ('line_start',1),('line_end',False)]:
            d=copy.deepcopy(original);d['pairs'][0]['a'][campo]=valor;casos.append(d)
        d=copy.deepcopy(original);d['pairs'][0]['b']['file']=0;casos.append(d)
        d=copy.deepcopy(original);d['pairs']*=2;casos.append(d)
        d=copy.deepcopy(original);d['pairs'][0]['tokens']=True;casos.append(d)
        d=copy.deepcopy(original);d['files'][0]['path']='/tmp/otra.cls';casos.append(d)
        d=copy.deepcopy(original);d['files'][0]['bytes']+=1;casos.append(d)
        d=copy.deepcopy(original);d['files']=[];casos.append(d)
        for d in casos:
            with self.subTest(datos=d), self.assertRaises(ccom.ErrorProtocolo):
                interpretar(json.dumps(d),FUENTE,0,docs,5)
        for raw in ('sin JSON','{"schema_version":1,"schema_version":1}', 'NaN'):
            with self.assertRaises(ccom.ErrorProtocolo): interpretar(raw,FUENTE,0,docs,5)

    def test_no_acepta_resultados_parciales(self):
        _,docs=empaquetar(SOURCES)
        for status in ('limit','error','lexical_error'):
            d=informe();d.update(status=status,exit_code=1,explanation='fallo explícito')
            with self.assertRaises(ccom.ErrorProtocolo): interpretar(json.dumps(d),FUENTE,1,docs,5)
            d['pairs']=[]
            with self.assertRaises(LimiteAnalisis if status=='limit' else ValueError):
                interpretar(json.dumps(d),FUENTE,1,docs,5)

    def test_flex_rechaza_cadenas_mayusculas_y_errores_de_rango(self):
        for extra in (' @',' "cadena"',' ABC',' 2147483649',' #[ abierto'):
            with self.subTest(extra=extra),self.assertRaisesRegex(ValueError,'lexicos'):
                detectar({Path('/a'):TEXT+extra,Path('/b'):TEXT},5)

    def test_limites_cancelacion_y_ccom_incompatible(self):
        with self.assertRaises(Cancelado): detectar(SOURCES,5,lambda:True)
        for minimo in (True,4,100001):
            with self.assertRaises(ValueError): detectar({},minimo)
        with self.assertRaises(LimiteAnalisis): detectar({Path(f'/{i}'):'' for i in range(201)})
        with self.assertRaises(LimiteAnalisis): detectar({Path('/a'):'é'*1000001})
        with self.assertRaises(ValueError): empaquetar({Path('/tmp/a'):'',Path('/tmp/../tmp/a'):''})
        with patch.object(ccom,'ejecutable',return_value=None):
            with self.assertRaisesRegex(ValueError,'no disponible'): detectar(SOURCES)
        with tempfile.TemporaryDirectory() as tmp:
            executable=Path(tmp)/'viejo'
            executable.write_text(f'#!{sys.executable}\nprint("sin JSON")\n');executable.chmod(0o755)
            with self.assertRaisesRegex(ccom.ErrorProtocolo,'JSON'):
                detectar(SOURCES,root=tmp,configurado=str(executable))

    def test_paquete_ordenado_independiente_y_sin_archivos_reales(self):
        paquete,docs=empaquetar(dict(reversed(list(SOURCES.items()))))
        self.assertTrue(paquete.startswith(b'CCL1'))
        self.assertEqual([p for p,_,_ in docs],list(SOURCES))
        self.assertEqual(detectar(SOURCES,5),detectar(dict(reversed(list(SOURCES.items()))),5))
        self.assertEqual(detectar({},5),[])

    def test_cancelacion_y_plazo_matan_hijo_y_limpian_paquete(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp=Path(tmp);ready=tmp/'ready';script=tmp/'lento'
            script.write_text(f'#!{sys.executable}\nimport pathlib,sys,time,os\n'
                              f'pathlib.Path({str(ready)!r}).write_text(str(os.getpid())+"\\n"+sys.argv[-1])\n'
                              'time.sleep(30)\n')
            script.chmod(0o755)
            for cancel in (True,False):
                ready.unlink(missing_ok=True)
                with self.assertRaises(InterruptedError if cancel else LimiteServicio):
                    consultar(b'CCL1\0\0\0\0',['--clones-json','5'],ejecutable=str(script),
                              cancelar=lambda:cancel and ready.exists(),plazo=.3)
                pid,path=ready.read_text().splitlines()
                self.assertFalse(Path('/proc',pid).exists(),'El hijo debe recogerse')
                self.assertFalse(Path(path).parent.exists(),'El paquete temporal debe eliminarse')


if __name__=='__main__':unittest.main()
