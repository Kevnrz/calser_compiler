#!/usr/bin/env python3
"""Pruebas del control de ejecucion; no lanza recursivamente las suites reales."""
import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('calser_runner', ROOT / 'tools/probar.py')
runner = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = runner
spec.loader.exec_module(runner)


class RunnerTests(unittest.TestCase):
    def invoke(self, args, codes=()):
        output = io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            with patch.object(runner, 'execute', side_effect=codes) as execute:
                code = runner.main(args)
        return code, output.getvalue(), execute

    def test_listing_does_not_build_or_run(self):
        code, output, execute = self.invoke(['--listar'])
        self.assertEqual(code, 0)
        self.assertIn('matrices', output)
        execute.assert_not_called()

    def test_selection_builds_once_and_deduplicates(self):
        code, output, execute = self.invoke(['ayuda', 'tiempo', 'ayuda'], [0, 0, 0])
        self.assertEqual(code, 0)
        self.assertEqual(execute.call_args_list[0].args[0], ('make', '--no-print-directory', 'ccom'))
        self.assertEqual(execute.call_count, 3)
        self.assertIn('2 aprobadas, 0 fallidas, 0 omitidas', output)

    def test_build_failure_runs_no_suite(self):
        code, output, execute = self.invoke(['matrices'], [2])
        self.assertEqual(code, 1)
        self.assertEqual(execute.call_count, 1)
        self.assertIn('FAIL preparacion', output)

    def test_failure_continues_by_default(self):
        code, output, execute = self.invoke(['--sin-compilar', 'ayuda', 'tiempo'], [1, 0])
        self.assertEqual(code, 1)
        self.assertEqual(execute.call_count, 2)
        self.assertIn('1 aprobadas, 1 fallidas, 0 omitidas', output)

    def test_fail_fast(self):
        code, output, execute = self.invoke(['--sin-compilar', '--detener-al-fallar', 'ayuda', 'tiempo'], [None])
        self.assertEqual(code, 1)
        self.assertEqual(execute.call_count, 1)
        self.assertIn('0 aprobadas, 1 fallidas, 1 omitidas', output)

    def test_negative_case_expected_and_mismatch(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / 'caso con espacios.cls'
            source.write_text('invalido')
            args = ['--sin-compilar', '--caso', str(source), '--codigo-esperado', '1']
            code, output, execute = self.invoke(args, [1])
            self.assertEqual(code, 0)
            command = execute.call_args.args[0]
            self.assertIn(str(source), command)
            self.assertFalse(Path(command[-1]).parent.exists())
            self.assertEqual(source.read_text(), 'invalido')
            self.assertEqual(self.invoke(args, [0])[0], 1)
            self.assertEqual(self.invoke(args, [None])[0], 1)

    def test_invalid_arguments(self):
        for args in (['desconocida'], ['--codigo-esperado', '1'],
                     ['--timeout', '0'], ['--timeout', 'nan'], ['--timeout', 'inf'],
                     ['--caso', 'inexistente.cls'], ['--caso', 'a.cls', 'ayuda']):
            with self.subTest(args=args), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as caught:
                    runner.main(args)
                self.assertEqual(caught.exception.code, 2)

    def test_process_errors_and_working_directory(self):
        options = type('Options', (), {'timeout': 2, 'detalle': False})()
        with contextlib.redirect_stderr(io.StringIO()):
            for error in (FileNotFoundError('no existe'), subprocess.TimeoutExpired(['test'], 2)):
                with patch.object(runner.subprocess, 'run', side_effect=error):
                    self.assertIsNone(runner.execute(['test'], options))
        with patch.object(runner.subprocess, 'run', return_value=subprocess.CompletedProcess(['test'], 0, '', '')) as run:
            self.assertEqual(runner.execute(['test'], options), 0)
            self.assertEqual(run.call_args.kwargs['cwd'], ROOT)
            self.assertEqual(run.call_args.kwargs['env']['PYTHONDONTWRITEBYTECODE'], '1')


if __name__ == '__main__':
    unittest.main()
