FROM appelmar/gdalcubes

# install R and a few packages
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
RUN add-apt-repository 'deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran35/'
RUN apt update -q && DEBIAN_FRONTEND=noninteractive apt install -q -y r-base
RUN Rscript -e "install.packages(c('devtools'))"

# TODO: replace by installing from github
RUN cd /opt/gdalcubes/Rpkg &&  R CMD INSTALL gdalcubes

CMD /bin/bash


# docker build -t appelmar/gdalcubes_demo .