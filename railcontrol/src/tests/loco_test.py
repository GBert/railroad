# locolist

def test_cmd_getlocolist_no_locos(service):
    response = service.cmd('getlocolist')

    assert response.headers['Content-Type'] == 'text/csv; charset=utf-8'
    assert not response.text


# locosave

def test_cmd_locosave_no_data(service):
    response = service.cmd('locosave')

    assert 'eControl does not exist' in response.text

# loco

def test_cmd_loco_without_loco(service):
    response = service.cmd('loco')
    assert 'Please select a locomotive' in response.text


def test_cmd_loco_with_loco_does_not_exist(service):
    response = service.cmd('loco', loco=1)
    assert 'Locomotive does not exist' in response.text
