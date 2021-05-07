FROM thewtex/opengl:ubuntu2004

WORKDIR /root/

# prepare for the opengl xvfb ffmpeg ..
RUN apt-get -yqq update && \
apt-get -yq install build-essential git pkg-config libgl1-mesa-dev libpthread-stubs0-dev libjpeg-dev libxml2-dev libtiff5-dev libgdal-dev libpoppler-dev libdcmtk-dev libgstreamer1.0-dev libgtk2.0-dev libcairo2-dev libpoppler-glib-dev libxrandr-dev libxinerama-dev curl wget mkvtoolnix xvfb ffmpeg cmake && \
apt-get autoremove -y && \
apt-get clean -y && \
pip install -i https://pypi.tuna.tsinghua.edu.cn/simple --upgrade pip && \
ln -s /usr/bin/ffmpeg /usr/bin/avconv && \
ln -s /usr/bin/python3 /usr/bin/python

# compile cmake, install cmake instead
# RUN version=3.15 && \
# build=3 && \
# mkdir ~/temp && \
# cd ~/temp && \
# wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz && \
# tar -xzvf cmake-$version.$build.tar.gz && \
# cd cmake-$version.$build/ && \
# ./bootstrap && \
# make -j4 && \
# make install

# compile OpenSceneGraph, use osg.7z instead
# RUN cd ~/ && \
# git clone https://github.com/openscenegraph/OpenSceneGraph && \
# cd OpenSceneGraph && \
# git checkout OpenSceneGraph-3.6.3 && \
# mkdir build && \
# cd build && \
# cmake ../ && \
# make -j4 && \
# make install && \
# ldconfig

COPY ./PlayerLib ./PlayerLib

# prepare for the esmini player environment
RUN cd /root/PlayerLib/esmini && \
#git clone https://github.com/TomcatCbn/esmini-self && \
mv ./externals/osi.7z ./externals/OSI/ && \
mv ./externals/osg.7z ./externals/OpenSceneGraph/ && \
mv ./externals/sumo.7z ./externals/SUMO/ && \
mv ./externals/googletest.7z ./externals/googletest/ && \
mkdir build && \
cd build && \
cmake .. && \
make -j4

ENV DISPLAY :1.0
#ENV LENGTH 20
#ENV RESOLUTION_X 320
#ENV RESOLUTION_Y 240
#ENV FRAMERATE 20

#export ESMINI_PATH="/root/esmini-self/build/EnvironmentSimulator/Applications" && \
#export PATH="$PATH:$ESMINI_PATH/esmini:$ESMINI_PATH/odrviewer:$ESMINI_PATH/replayer"

# -------------- for open scenario generate server -----------
COPY ./requirements.txt ./
RUN cd /root/ && \
pip install setuptools && \
pip install -r requirements.txt
COPY ./PlayerServer ./PlayerServer
# -------------- for open scenario generate server -----------

# copy entrypoint.sh
COPY ./entrypoint.sh /root/
RUN ["chmod", "+x", "/root/entrypoint.sh"]

# set environment variables
ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

# run entrypoint.sh
ENTRYPOINT ["/root/entrypoint.sh"]

