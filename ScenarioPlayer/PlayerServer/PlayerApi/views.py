import json
# Create your views here.
from django.http import HttpResponse
from rest_framework.decorators import api_view
from rest_framework.renderers import JSONRenderer

from .internal.simulator.esmini import record_gif


class JSONResponse(HttpResponse):
    """
    An HttpResponse that renders its content into JSON.
    """

    def __init__(self, data, **kwargs):
        content = JSONRenderer().render(data)
        kwargs['content_type'] = 'application/json'
        super(JSONResponse, self).__init__(content, **kwargs)


@api_view(['POST'])
def api_request_record2gif(request):
    xodr_path = request.data['xodr']
    xosc_path = request.data['xosc']
    total_time = request.data['totalTime']
    print('xodr_path=%s, xosc_path=%s' % (xodr_path, xosc_path))
    if not xosc_path or not xodr_path:
        return HttpResponse(content=b'params error')
    gif_path = record_gif(xodr_path, xosc_path,
                          length=int(total_time))
    temp = gif_path.split('/')
    gif_path = temp[len(temp) - 1]
    print(gif_path)
    res = {'gifName': gif_path, 'code': 0}
    return JSONResponse(json.dumps(res, ensure_ascii=False).encode('utf-8'))
