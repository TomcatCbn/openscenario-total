# pull official base image
FROM python:3.9.5

# set work directory
WORKDIR /usr/src/app

# set environment variables
ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

# install psycopg2 dependencies
#RUN apk update && \
#apk add postgresql-dev gcc python3-dev musl-dev tzdata gcc make freetype-dev gfortran musl-dev libgcc libquadmath musl libgfortran lapack-dev

COPY ./requirements.txt .
# install dependencies
RUN pip install --upgrade pip && \
apt-get update && \
apt-get -yq install netcat && \
pip install -r requirements.txt
# copy entrypoint.sh
#COPY ./entrypoint.sh .

# copy project
COPY . .

# run entrypoint.sh
ENTRYPOINT ["/usr/src/app/entrypoint.sh"]
