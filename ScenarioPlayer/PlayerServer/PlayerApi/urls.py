from django.urls import re_path

from .views import api_request_record2gif

urlpatterns = [
    re_path(r'^2gif$', api_request_record2gif),
]
