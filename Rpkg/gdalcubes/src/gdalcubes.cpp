
#include <gdalcubes/gdalcubes.h>


// [[Rcpp::depends(RcppProgress)]]
#include <Rcpp.h>
#include <progress.hpp>
#include <progress_bar.hpp>

using namespace Rcpp;




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
  //Rcout << (*x)->to_string() << std::endl;

  Rcpp::XPtr< std::shared_ptr<image_collection_cube> > p(x, true) ;

  Rcout << (*x)->to_string() << std::endl;

  // TODO also return size / band information as members of an output list
  return p;
}


// [[Rcpp::export]]
SEXP libgdalcubes_create_reduce_cube(SEXP inptr, std::string reducer) {



  Rcpp::XPtr< std::shared_ptr<cube> > pin(inptr);

  std::shared_ptr<reduce_cube>* x = new std::shared_ptr<reduce_cube>( std::make_shared<reduce_cube>(*pin, reducer));
  Rcpp::XPtr< std::shared_ptr<reduce_cube> > p(x, true) ;

  Rcout << (*x)->to_string() << std::endl;

  Rcout << (*x)->make_constructible_json().dump(2) << std::endl;
    // TODO also return size / band information as members of an output list
  return p;
}



// [[Rcpp::export]]
void libgdalcubes_eval_reduce_cube(SEXP inptr, std::string outfile, std::string of) {
  Rcpp::XPtr< std::shared_ptr<reduce_cube> > pin(inptr);
  (*pin)->write_gdal_image(outfile, of);
}




// [[Rcpp::export]]
SEXP libgdalcubes_create_stream_cube(SEXP inptr, std::string cmd, std::vector<int> chunk_size) {

  Rcpp::XPtr< std::shared_ptr<image_collection_cube> > pin(inptr);

  std::shared_ptr<stream_cube>* x = new std::shared_ptr<stream_cube>( std::make_shared<stream_cube>(*pin, cmd));
  (*pin)->set_chunk_size(chunk_size[0], chunk_size[1], chunk_size[2]);

  Rcpp::XPtr< std::shared_ptr<stream_cube> > p(x, true) ;

  Rcout << (*x)->to_string() << std::endl;

  // TODO also return size / band information as members of an output list
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


