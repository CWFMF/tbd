FROM php:7.2-apache AS wxshield
RUN apt-get update
RUN apt-get install -y libpq-dev \
  && docker-php-ext-configure pgsql -with-pgsql=/usr/local/pgsql \
  && docker-php-ext-install pdo pdo_pgsql pgsql
RUN apt-get install -y wget unzip
WORKDIR /var/www/html/wxshield/
RUN rm -rf bootstrap* \
  && wget https://github.com/twbs/bootstrap/releases/download/v3.4.1/bootstrap-3.4.1-dist.zip \
  && unzip bootstrap-3.4.1-dist.zip \
  && mv bootstrap-3.4.1-dist bootstrap \
  && rm bootstrap-3.4.1-dist.zip
WORKDIR /var/www/html/wxshield/js/
# no idea how files exist already but remove them first
RUN rm -rf *.zip jquery* jspdf* jsPDF* \
  && wget https://code.jquery.com/jquery-1.11.3.js \
  && wget https://code.jquery.com/jquery-1.11.3.min.js \
  && wget https://github.com/jquery-validation/jquery-validation/archive/1.11.1.zip \
  && unzip 1.11.1.zip \
  && mv jquery-validation-1.11.1/jquery.validate.js ./ \
  && wget https://github.com/MrRio/jsPDF/archive/v1.2.61.zip \
  && unzip v1.2.61.zip \
  && (mkdir -p jspdf && mv jsPDF-1.2.61/dist/jspdf.min.js jspdf/jspdf.js) \
  && rm -rf ./jsPDF-1.2.61* ./*.zip
# FIX: this shouldn't be static
COPY settings.ini /var/www/html/wxshield
COPY WeatherSHIELD/gui/ /var/www/html/wxshield/
