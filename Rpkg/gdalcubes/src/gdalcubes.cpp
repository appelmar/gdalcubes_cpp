
#include <gdalcubes/gdalcubes.h>


// [[Rcpp::depends(RcppProgress)]]
#include <Rcpp.h>
#include <progress.hpp>
#include <progress_bar.hpp>

using namespace Rcpp;

/**
 * TODO: return more cube information. including bands, spatial / temporal footprint, ...
 */


struct progress_simple_R : public progress {
  std::shared_ptr<progress> get() override { return std::make_shared<progress_simple_R>(); }
  void set(double p) override {
    if (!_rp) _rp = new Progress(100,true);
    _m.lock();
    double p_old = _p;
    _p = p;
    _rp->update((int)(_p*100));
    _m.unlock();
  };
  void increment(double dp) override {
    if (!_rp) _rp = new Progress(100,true);
    set(_p + dp);
  }
  virtual void finalize() override {
    _rp->update(100);
  }


  progress_simple_R() : _p(0), _rp(nullptr) {}

  ~progress_simple_R(){
    if (_rp) delete _rp;
  }

private:
  std::mutex _m;
  double _p;
  Progress *_rp;
};


// [[Rcpp::export]]
void libgdalcubes_version() {
  Rcout << "gdalcubes " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << std::endl;
}


// [[Rcpp::export]]
void libgdalcubes_init() {
  config::instance()->gdalcubes_init();
  config::instance()->set_default_progress_bar(std::make_shared<progress_simple_R>());
}

// [[Rcpp::export]]
void libgdalcubes_cleanup() {
  config::instance()->gdalcubes_cleanup();
}


Rcpp::List basic_cube_info( std::shared_ptr<cube> x ) {

  Rcpp::CharacterVector d_name(3);
  Rcpp::NumericVector d_low(3);
  Rcpp::NumericVector d_high(3);
  Rcpp::IntegerVector d_n(3);
  Rcpp::IntegerVector d_chunk(3);



  d_name[0] = "t";
  d_low[0] = x->st_reference().t0().to_double();
  d_high[0] = x->st_reference().t1().to_double();
  d_n[0] = x->st_reference().nt();
  d_chunk[0] = x->chunk_size()[0];

  d_name[1] = "y";
  d_low[1] = x->st_reference().bottom();
  d_high[1] = x->st_reference().top();
  d_n[1] = x->st_reference().ny();
  d_chunk[1] = x->chunk_size()[1];

  d_name[2] = "x";
  d_low[2] = x->st_reference().left();
  d_high[2] = x->st_reference().right();
  d_n[2] = x->st_reference().nx();
  d_chunk[2] = x->chunk_size()[2];

  Rcpp::DataFrame dims =
    Rcpp::DataFrame::create(Rcpp::Named("name")=d_name,
                            Rcpp::Named("low")=d_low,
                            Rcpp::Named("high")=d_high,
                            Rcpp::Named("size")=d_n,
                            Rcpp::Named("chunk_size")=d_chunk);


  // Rcpp::List stref = Rcpp::List::create(
  //   Rcpp::Named("space") =
  //     Rcpp::List::create(
  //       Rcpp::Named("win") =
  //         Rcpp::NumericVector::create(
  //           _["left"] = x->st_reference().left() ,
  //           _["right"] = x->st_reference().right() ,
  //           _["top"] = x->st_reference().top() ,
  //           _["bottom"] = x->st_reference().bottom()),
  //       Rcpp::Named("size") =
  //         Rcpp::NumericVector::create(
  //           _["nx"] = x->st_reference().nx() ,
  //           _["ny"] = x->st_reference().ny()),
  //       Rcpp::Named("proj") = x->st_reference().proj()),
  //   Rcpp::Named("time") =
  //     Rcpp::List::create(
  //       Rcpp::Named("t0") = x->st_reference().t0().to_string() ,
  //       Rcpp::Named("t1") = x->st_reference().t1().to_string() ,
  //       Rcpp::Named("dt") = x->st_reference().dt().to_string()));

  Rcpp::CharacterVector b_names(x->bands().count(), "");
  Rcpp::CharacterVector b_type(x->bands().count(), "");
  Rcpp::NumericVector b_offset(x->bands().count(), NA_REAL);
  Rcpp::NumericVector b_scale(x->bands().count(), NA_REAL);
  Rcpp::NumericVector b_nodata(x->bands().count(), NA_REAL);
  Rcpp::CharacterVector b_unit(x->bands().count(), "");

  for (uint16_t i=0; i<x->bands().count(); ++i) {
    b_names[i] = x->bands().get(i).name;
    b_type[i] = x->bands().get(i).type;
    b_offset[i] = x->bands().get(i).offset;
    b_scale[i] = x->bands().get(i).scale;
    b_nodata[i] = (x->bands().get(i).no_data_value.empty())? NA_REAL : std::stod(x->bands().get(i).no_data_value);
    b_unit[i] = x->bands().get(i).unit;
  }

  Rcpp::DataFrame bands =
    Rcpp::DataFrame::create(Rcpp::Named("name")=b_names,
                            Rcpp::Named("type")=b_type,
                            Rcpp::Named("offset")=b_offset,
                            Rcpp::Named("scale")=b_scale,
                            Rcpp::Named("nodata")=b_nodata,
                            Rcpp::Named("unit")=b_unit);

  return Rcpp::List::create(Rcpp::Named("bands") = bands,
                            Rcpp::Named("dimensions") = dims,
                            Rcpp::Named("proj") = x->st_reference().proj(),
                            Rcpp::Named("graph") = x->make_constructible_json().dump(),
                            Rcpp::Named("size") = Rcpp::IntegerVector::create(x->size()[0], x->size()[1], x->size()[2], x->size()[3]));

}


// [[Rcpp::export]]
SEXP libgdalcubes_open_image_collection(std::string filename) {
  std::shared_ptr<image_collection>* x = new std::shared_ptr<image_collection>( std::make_shared<image_collection>(filename));
  Rcout << (*x)->to_string() << std::endl;

  Rcpp::XPtr< std::shared_ptr<image_collection> > p(x, true) ;
  return p;
}

// [[Rcpp::export]]
SEXP libgdalcubes_create_image_collection_cube(std::string filename, std::string view) {

  cube_view v = cube_view::read_json_string(view);
  std::shared_ptr<image_collection_cube>* x = new std::shared_ptr<image_collection_cube>( std::make_shared<image_collection_cube>(filename, v));
  Rcpp::XPtr< std::shared_ptr<image_collection_cube> > p(x, true) ;


  Rcpp::List cube_info = basic_cube_info(*x);

  return Rcpp::List::create(Rcpp::Named("type") = "image_collection_cube",
                            Rcpp::Named("bands") = cube_info["bands"],
                            Rcpp::Named("dimensions") = cube_info["dimensions"],
                            Rcpp::Named("proj") = cube_info["proj"],
                            Rcpp::Named("graph") = cube_info["graph"],
                            Rcpp::Named("size") = cube_info["size"],
                            Rcpp::Named("xptr") = p);

  // TODO also return size / band information as members of an output list
  return p;
}


// [[Rcpp::export]]
SEXP libgdalcubes_create_reduce_cube(Rcpp::List incube, std::string reducer) {
  Rcpp::XPtr< std::shared_ptr<cube> > pin = incube["xptr"];
  std::shared_ptr<reduce_cube>* x = new std::shared_ptr<reduce_cube>( std::make_shared<reduce_cube>(*pin, reducer));
  Rcpp::XPtr< std::shared_ptr<reduce_cube> > p(x, true) ;

  Rcpp::List cube_info = basic_cube_info(*x);

  return Rcpp::List::create(Rcpp::Named("type") = "reduce_cube",
                            Rcpp::Named("bands") = cube_info["bands"],
                            Rcpp::Named("dimensions") = cube_info["dimensions"],
                            Rcpp::Named("proj") = cube_info["proj"],
                            Rcpp::Named("graph") = cube_info["graph"],
                            Rcpp::Named("size") = cube_info["size"],
                            Rcpp::Named("xptr") = p);
}



// [[Rcpp::export]]
void libgdalcubes_eval_reduce_cube(Rcpp::List incube, std::string outfile, std::string of) {
  Rcpp::XPtr< std::shared_ptr<reduce_cube> > pin = incube["xptr"];
  (*pin)->write_gdal_image(outfile, of);
}


// [[Rcpp::export]]
SEXP libgdalcubes_create_stream_cube(Rcpp::List incube, std::string cmd, std::vector<int> chunk_size) {

  Rcpp::XPtr< std::shared_ptr<image_collection_cube> > pin = incube["xptr"];


  std::shared_ptr<stream_cube>* x = new std::shared_ptr<stream_cube>( std::make_shared<stream_cube>(*pin, cmd));
  (*pin)->set_chunk_size(chunk_size[0], chunk_size[1], chunk_size[2]);

  Rcpp::XPtr< std::shared_ptr<stream_cube> > p(x, true) ;



  Rcpp::List cube_info = basic_cube_info(*x);

  return Rcpp::List::create(Rcpp::Named("type") = "stream_cube",
                            Rcpp::Named("bands") = cube_info["bands"],
                            Rcpp::Named("dimensions") = cube_info["dimensions"],
                            Rcpp::Named("proj") = cube_info["proj"],
                            Rcpp::Named("graph") = cube_info["graph"],
                            Rcpp::Named("size") = cube_info["size"],
                            Rcpp::Named("xptr") = p);
  return p;
}


// [[Rcpp::export]]
void libgdalcubes_set_threads(IntegerVector n) {
  if (n[0] > 1) {
    config::instance()->set_default_chunk_processor(std::dynamic_pointer_cast<chunk_processor>(std::make_shared<chunk_processor_multithread>(n[0])));
  }
  else {
    config::instance()->set_default_chunk_processor(std::dynamic_pointer_cast<chunk_processor>(std::make_shared<chunk_processor_singlethread>()));
  }
}


