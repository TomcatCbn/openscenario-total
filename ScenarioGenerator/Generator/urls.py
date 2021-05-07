from django.urls import re_path

from .views import convert_open_scenario

urlpatterns = [
    re_path(r'^scenario$', convert_open_scenario),
]
