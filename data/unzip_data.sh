#!/bin/bash

python get_data.py

tar xvfz semcor.tar.gz
unzip 20_news_date.zip

cd movies
tar xvfz german_movies.tar.gz
tar xvfz pang_lee.tar.gz