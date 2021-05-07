#!/bin/sh

if [ "$DATABASE" = "scenario" ]
then
    echo "Waiting for postgres scenario..."

    while ! nc -z $SQL_HOST $SQL_PORT; do
      sleep 0.1
    done

    echo "PostgreSQL started"
fi

if [ "$SCENARIO_PLAYER" = "esmini" ]
then
    echo "Waiting for scenario player ..."

    while ! nc -z $SCENARIO_HOST $SCENARIO_PORT; do
      sleep 0.1
    done

    echo "Scenario player started"
fi

echo $PYTHONPATH

python -V

python manage.py flush --no-input
python manage.py migrate

exec "$@"
