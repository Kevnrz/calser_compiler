import pytest

from ide.nucleo.correccion import (
    EdicionTexto,
    aplicar_ediciones,
    proponer_reparaciones,
)


PROGRAMA = "F main [] : void ;\nE ;\n."


def test_programa_correcto_no_recibe_reparaciones():
    r = proponer_reparaciones(PROGRAMA)
    assert r.estado == "sin_error"
    assert r.propuestas == ()


def test_inserta_punto_y_coma_antes_de_una_sentencia_no_ambigua():
    texto = "F main [] : void ;\nVm x : int\nE ;\n."
    r = proponer_reparaciones(texto)
    assert r.estado == "propuestas"
    assert len(r.propuestas) == 1
    p = r.propuestas[0]
    assert p.codigo == "INSERCION_PUNTO_COMA"
    assert p.validacion.aceptado
    assert p.automatica_segura
    assert aplicar_ediciones(texto, p.ediciones) == "F main [] : void ;\nVm x : int\n;E ;\n."
    assert proponer_reparaciones(aplicar_ediciones(texto, p.ediciones)).estado == "sin_error"


def test_punto_y_coma_en_eof_es_manual_y_no_cierra_la_funcion_sola():
    texto = "F main [] : void ;\nVm x : int = 3"
    r = proponer_reparaciones(texto)
    assert r.estado == "propuestas"
    p = next(p for p in r.propuestas if p.codigo == "INSERCION_PUNTO_COMA")
    assert p.validacion.error_resuelto
    assert not p.validacion.aceptado  # Falta además el punto de cierre de función.
    assert not p.automatica_segura


def test_punto_final_en_eof_es_propuesta_manual():
    texto = "F main [] : void ;\nE ;"
    r = proponer_reparaciones(texto)
    assert [p.codigo for p in r.propuestas] == ["INSERCION_PUNTO_FINAL"]
    assert r.propuestas[0].validacion.aceptado
    assert not r.propuestas[0].automatica_segura


def test_corchete_y_parentesis_solo_se_proponen_si_reanalisis_avanza():
    corchete = "F main [] : void ;\nC [verdadero ;\nE ;\n."
    parentesis = "F main [] : void ;\nVm x : int = (3 + 2 ;\nE ;\n."
    rc = proponer_reparaciones(corchete)
    rp = proponer_reparaciones(parentesis)
    assert any(p.codigo == "INSERCION_CORCHETE_CIERRE" for p in rc.propuestas)
    assert any(p.codigo == "INSERCION_PARENTESIS_CIERRE" and p.validacion.aceptado
               for p in rp.propuestas)
    assert all(not p.automatica_segura for p in rc.propuestas + rp.propuestas)


def test_no_inventa_operando_ni_arregla_expresion_incompleta():
    texto = "F main [] : void ;\nx = 3 + ;\nE ;\n."
    r = proponer_reparaciones(texto)
    assert r.estado == "sin_propuesta"


def test_errores_lexicos_bloquean_reparaciones():
    texto = "F main [] : void ;\nVm x : int @\nE ;\n."
    r = proponer_reparaciones(texto)
    assert r.estado == "bloqueado"
    assert not r.propuestas
    assert "errores_lexicos" in r.limites


def test_comentarios_no_se_modifican_y_unicode_no_desplaza_offsets():
    texto = "F main [] : void ;\n#[ ñ ; ]#\r\nVm x : int\r\nE ;\r\n."
    r = proponer_reparaciones(texto)
    assert r.propuestas
    p = r.propuestas[0]
    edit = p.ediciones[0]
    assert texto[edit.inicio:edit.fin] == ""
    corregido = aplicar_ediciones(texto, p.ediciones)
    assert "#[ ñ ; ]#" in corregido
    assert "\r\n;E ;" in corregido


def test_aplicacion_de_ediciones_es_derecha_a_izquierda_y_rechaza_choques():
    texto = "abcdef"
    ediciones = (EdicionTexto(1, 2, "B"), EdicionTexto(4, 4, "!"))
    assert aplicar_ediciones(texto, ediciones) == "aBcd!ef"
    with pytest.raises(ValueError):
        aplicar_ediciones(texto, (EdicionTexto(2, 2, ";"), EdicionTexto(2, 2, ".")))
    with pytest.raises(ValueError):
        aplicar_ediciones(texto, (EdicionTexto(1, 4, "x"), EdicionTexto(3, 5, "y")))


def test_hash_y_longitud_identifican_el_texto_origen():
    texto = PROGRAMA
    r = proponer_reparaciones(texto)
    assert r.longitud_texto == len(texto)
    assert len(r.huella_texto) == 64
    assert all(p.huella_texto == r.huella_texto for p in r.propuestas)


def test_limite_de_candidatos_se_reporta():
    # Una entrada de EOF con varios terminales de continuación no debe exceder
    # el límite configurado ni fingir que el análisis terminó correctamente.
    texto = "F main [] : void ;\nVm x : int = 3"
    r = proponer_reparaciones(texto, max_candidatos=1)
    assert len(r.propuestas) <= 1
    assert r.limites == ()  # Solo hay un terminal reparable esperado: ';'.
