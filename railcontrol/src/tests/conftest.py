import os
import subprocess
from pathlib import Path

import pytest
import requests
import time


class Client:
    def __init__(self, url: str):
        self.url = url
        self.session = requests.Session()

    def cmd(self, cmd: str, **kwargs):
        params = kwargs.copy()
        params['cmd'] = cmd
        response = self.session.get(self.url, params=params)
        response.raise_for_status()
        return response

    def ping(self):
        print('Pinging', self.url)
        try:
            response = self.session.get(self.url, timeout=10)
            response.raise_for_status()
        except requests.exceptions.ConnectionError:
            return False

        return True


@pytest.fixture
def service(tmpdir: Path, port: int = 8022):
    config = "dbfilename = 'railcontrol.sqlite'\n" \
            f"webserverport = {port}\n" \
            "numkeepbackups = 0\n"

    configfile = tmpdir / 'config.conf'
    configfile.write_text(config, 'UTF-8')

    url = f"http://localhost:{port}"

    command = [os.path.join(os.getcwd(), 'railcontrol'), '--config', str(configfile)]
    print("Starting railcontrol")
    with subprocess.Popen(command, cwd=tmpdir) as proc:
        print("Started", proc.pid)
        client = Client(url)

        for _ in range(10):
            print("Polling", proc.pid)
            if proc.poll():
                pytest.fail(f"Server exited with code {proc.returncode}")
            if client.ping():
                yield client
                break
            time.sleep(0.5)
        else:
            proc.kill()
            pytest.fail("Could not connect to service")

        proc.kill()
