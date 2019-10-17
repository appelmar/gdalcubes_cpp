

#include "vector_queries.h"
#include <thread>

namespace gdalcubes {

std::vector<std::vector<double>> vector_queries::query_points(std::shared_ptr<cube> cube, std::vector<double> x,
                                                              std::vector<double> y, std::vector<std::string> t,
                                                              std::string srs) {
    // make sure that x, y, and t have same size
    if (x.size() != y.size() || y.size() != t.size()) {
        GCBS_ERROR("Point coordinate vectors x, y, t must have identical length");
        throw std::string("Point coordinate vectors x, y, t must have identical length");
    }

    if (x.empty()) {
        GCBS_ERROR("Point coordinate vectors x, y, t must have length > 0");
        throw std::string("Point coordinate vectors x, y, t must have length > 0");
    }

    if (!cube) {
        GCBS_ERROR("Invalid data cube pointer");
        throw std::string("Invalid data cube pointer");
    }

    uint32_t nthreads = config::instance()->get_default_chunk_processor()->max_threads();

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately

    // coordinate transformation
    if (cube->st_reference()->srs() != srs) {
        OGRSpatialReference srs_in;
        OGRSpatialReference srs_out;
        srs_in.SetFromUserInput(cube->st_reference()->srs().c_str());
        srs_out.SetFromUserInput(srs.c_str());

        if (!srs_in.IsSame(&srs_out)) {
            std::vector<std::thread> workers_transform;
            uint32_t n = (uint32_t)std::ceil(double(x.size()) / double(nthreads));  // points per thread

            for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
                workers_transform.push_back(std::thread([&cube, &srs, &srs_in, &srs_out, &x, &y, ithread, n](void) {
                    OGRCoordinateTransformation* coord_transform = OGRCreateCoordinateTransformation(&srs_in, &srs_out);

                    int begin = ithread * n;
                    int end = std::min(uint32_t(ithread * n + n), uint32_t(x.size()));
                    int count = end - begin;

                    // change coordinates in place, should be safe because vectors don't change their sizes
                    if (count > 0) {
                        if (coord_transform == nullptr || !coord_transform->Transform(count, x.data() + begin, y.data() + begin)) {
                            throw std::string("ERROR: coordinate transformation failed (from " + cube->st_reference()->srs() + " to " + srs + ").");
                        }
                    }
                    OCTDestroyCoordinateTransformation(coord_transform);
                }));
            }
            for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
                workers_transform[ithread].join();
            }
        }
    }

    // TODO: possible without additional copy?
    std::vector<double> it;  // array indexes
                             //    ix.resize(x.size());
                             //    iy.resize(x.size());
    it.resize(x.size());

    std::map<chunkid_t, std::vector<uint32_t>> chunk_index;

    std::vector<std::thread> workers_preprocess;
    std::mutex mtx;
    for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
        workers_preprocess.push_back(std::thread([&mtx, &cube, &x, &y, &t, &it, &chunk_index, ithread, nthreads](void) {
            for (uint32_t i = ithread; i < x.size(); i += nthreads) {
                coords_st st;

                st.s.x = x[i];
                st.s.y = y[i];

                // array coordinates
                x[i] = (x[i] - cube->st_reference()->left()) / cube->st_reference()->dx();
                //iy.push_back(cube->st_reference()->ny() - 1 - ((y[i] - cube->st_reference()->bottom()) / cube->st_reference()->dy()));  // top 0
                y[i] = (y[i] - cube->st_reference()->bottom()) / cube->st_reference()->dy();

                datetime dt = datetime::from_string(t[i]);
                if (dt.unit() > cube->st_reference()->dt().dt_unit) {
                    dt.unit() = cube->st_reference()->dt().dt_unit;
                    GCBS_WARN("date / time of query point has coarser granularity than the data cube; converting '" + t[i] + "' -> '" + dt.to_string() + "'");
                } else {
                    dt.unit() = cube->st_reference()->dt().dt_unit;
                }
                duration delta = cube->st_reference()->dt();
                it[i] = (dt - cube->st_reference()->t0()) / delta;

                if (it[i] < 0 || it[i] >= cube->size_t() ||
                    x[i] < 0 || x[i] >= cube->size_x() ||
                    y[i] < 0 || y[i] >= cube->size_y()) {  // if point is outside of the cube
                    continue;
                }
                st.t = dt;
                chunkid_t c = cube->find_chunk_that_contains(st);

                mtx.lock();
                chunk_index[c].push_back(i);
                mtx.unlock();
            }
        }));
    }
    for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
        workers_preprocess[ithread].join();
    }

    std::vector<std::vector<double>> out;
    out.resize(cube->bands().count());
    for (uint16_t ib = 0; ib < out.size(); ++ib) {
        out[ib].resize(x.size(), NAN);
    }

    std::vector<chunkid_t> chunks;  // vector of keys in chunk_index
    for (auto iter = chunk_index.begin(); iter != chunk_index.end(); ++iter) {
        chunks.push_back(iter->first);
    }

    std::vector<std::thread> workers;
    for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
        workers.push_back(std::thread([&prg, &cube, &out, &chunk_index, &chunks, &x, &it, &y, ithread, nthreads](void) {
            for (uint32_t ic = ithread; ic < chunks.size(); ic += nthreads) {
                try {
                    if (chunks[ic] < cube->count_chunks()) {  // if chunk exists
                        std::shared_ptr<chunk_data> dat = cube->read_chunk(chunks[ic]);
                        if (!dat->empty()) {  // if chunk is not empty
                            // iterate over all query points within the current chunk
                            for (uint32_t i = 0; i < chunk_index[chunks[ic]].size(); ++i) {
                                double ixc = x[chunk_index[chunks[ic]][i]];
                                double iyc = y[chunk_index[chunks[ic]][i]];
                                double itc = it[chunk_index[chunks[ic]][i]];

                                int iix = ((int)std::floor(ixc)) % cube->chunk_size()[2];
                                int iiy = dat->size()[2] - 1 - (((int)std::floor(iyc)) % cube->chunk_size()[1]);
                                int iit = ((int)std::floor(itc)) % cube->chunk_size()[0];

                                // check to prevent out of bounds faults
                                if (iix < 0 || uint32_t(iix) >= dat->size()[3]) continue;
                                if (iiy < 0 || uint32_t(iiy) >= dat->size()[2]) continue;
                                if (iit < 0 || uint32_t(iit) >= dat->size()[1]) continue;

                                for (uint16_t ib = 0; ib < out.size(); ++ib) {
                                    out[ib][chunk_index[chunks[ic]][i]] = ((double*)dat->buf())[ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + iit * dat->size()[2] * dat->size()[3] + iiy * dat->size()[3] + iix];
                                }
                            }
                        }
                    }
                    prg->increment((double)1 / (double)chunks.size());
                } catch (std::string s) {
                    GCBS_ERROR(s);
                    continue;
                } catch (...) {
                    GCBS_ERROR("unexpected exception while processing chunk " + std::to_string(chunks[ic]));
                    continue;
                }
            }
        }));
    }
    for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
        workers[ithread].join();
    }
    prg->finalize();

    return out;
}

}  // namespace gdalcubes