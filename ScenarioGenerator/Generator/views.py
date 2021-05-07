from urllib.parse import quote
from django.http import HttpResponse, StreamingHttpResponse
# Create your views here.
from rest_framework.decorators import api_view
from rest_framework.renderers import JSONRenderer

from .internal.utils.fileutil import file_iterator
from .internal.traffic_stream_parse import handle


class JSONResponse(HttpResponse):
    """
    An HttpResponse that renders its content into JSON.
    """

    def __init__(self, data, **kwargs):
        content = JSONRenderer().render(data)
        kwargs['content_type'] = 'application/json'
        super(JSONResponse, self).__init__(content, **kwargs)


# class XMLResponse(HttpResponse):
#     def __init__(self, path, **kwargs):
#         render

@api_view(['GET'])
def convert_open_scenario(request):
    type = request.query_params.get('type')
    download = request.query_params.get('download')
    xord, xosc, gif = handle('gif' == type)
    print('xord=%s\nxosc=%s\ngif=%s' % (xord, xosc, gif))
    if 'xord' == type:
        content_type = 'application/xml'
        file_path = xord
    elif 'gif' == type:
        content_type = 'image/gif'
        file_path = gif
    else:
        content_type = 'application/xml'
        file_path = xosc

    if download:
        # 返回文件，可下载
        file = file_iterator(file_path)
        response = StreamingHttpResponse(file)
        response['Content-Type'] = content_type
        paths = file_path.split('/')
        response['Content-Disposition'] = 'attachment; filename={0}'.format(quote(paths[len(paths)-1]))
        return response
    else:
        with open(file_path, 'rb') as f:
            content = f.read()
        return HttpResponse(content=content, content_type=content_type)
