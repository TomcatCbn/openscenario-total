import json
import os

import requests

from ..constant import GENERATE_FILE_DIR

host = os.environ.get("SCENARIO_HOST", "scenarioplayer")
port = os.environ.get("SCENARIO_PORT", "5433")


def request_record2gif(xodr, xosc, total_time):
    url = 'http://%s:%s/api/record/2gif' % (host, port)
    temp = xodr.split('/')
    xodr_name = temp[len(temp) - 1]
    temp = xosc.split('/')
    xosc_name = temp[len(temp) - 1]

    params = {'xodr': xodr_name, 'xosc': xosc_name, 'totalTime': total_time}
    print('params=%s' % str(params))
    response = requests.post(url=url, json=params)
    res = json.loads(response.json())
    print(type(res))
    print(res['gifName'])
    return '%s%s' % (GENERATE_FILE_DIR, res['gifName'])
