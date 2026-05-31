from pathlib import Path
import json


package_dir = Path(__file__).resolve().parents[1]
repo_dir = package_dir.parent
data_dir = package_dir / 'things'
legacy_script_path = package_dir / 'auto-profile.py'


def data_path(name):
    return data_dir / name


def read_text_file(name):
    path = data_path(name)
    if not path.exists():
        return ''
    return path.read_text(encoding='utf-8')


def write_text_file(name, value):
    path = data_path(name)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding='utf-8')


def read_json_file(name, fallback):
    path = data_path(name)
    if not path.exists():
        return fallback
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except json.JSONDecodeError:
        return fallback


def write_json_file(name, value):
    path = data_path(name)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2), encoding='utf-8')


def read_lines_file(name):
    value = read_text_file(name).replace('\r\n', '\n')
    return [line.strip() for line in value.split('\n') if line.strip()]
