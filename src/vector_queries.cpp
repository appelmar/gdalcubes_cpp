

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

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately

    // transformation
    // TODO: parallelize
    if (cube->st_reference()->srs() != srs) {
        OGRSpatialReference srs_in;
        OGRSpatialReference srs_out;
        srs_in.SetFromUserInput(cube->st_reference()->srs().c_str());
        srs_out.SetFromUserInput(srs.c_str());

        if (!srs_in.IsSame(&srs_out)) {
            OGRCoordinateTransformation* coord_transform = OGRCreateCoordinateTransformation(&srs_in, &srs_out);

            // change coordinates in place, should be safe because vectors don't change their sizes
            if (coord_transform == NULL || !coord_transform->Transform(x.size(), x.data(), y.data())) {
                throw std::string("ERROR: coordinate transformation failed (from " + cube->st_reference()->srs() + " to " + srs + ").");
            }
            OCTDestroyCoordinateTransformation(coord_transform);
        }
    }

    std::vector<double> ix, iy, it;  // array indexes
    ix.reserve(x.size());
    iy.reserve(x.size());
    it.reserve(x.size());

    std::map<chunkid_t, std::vector<uint32_t>> chunk_index;

    // TODO: parallelize
    for (uint32_t i = 0; i < x.size(); ++i) {
        // array coordinates
        ix.push_back((x[i] - cube->st_reference()->left()) / cube->st_reference()->dx());
        //iy.push_back(cube->st_reference()->ny() - 1 - ((y[i] - cube->st_reference()->bottom()) / cube->st_reference()->dy()));  // top 0
        iy.push_back((y[i] - cube->st_reference()->bottom()) / cube->st_reference()->dy());

        datetime dt = datetime::from_string(t[i]);
        if (dt.unit() > cube->st_reference()->dt().dt_unit) {
            dt.unit() = cube->st_reference()->dt().dt_unit;
            GCBS_WARN("date / time of query point has coarser granularity than the data cube; converting '" + t[i] + "' -> '" + dt.to_string() + "'");
        } else {
            dt.unit() = cube->st_reference()->dt().dt_unit;
        }
        duration delta = cube->st_reference()->dt();
        it.push_back((dt - cube->st_reference()->t0()) / delta);

        if (it[i] < 0 || it[i] >= cube->size_t() ||
            ix[i] < 0 || ix[i] >= cube->size_x() ||
            iy[i] < 0 || iy[i] >= cube->size_y()) {  // if point is outside of the cube
            continue;
        }

        // find out in which chunk
        coords_st st;
        st.t = dt;
        st.s.x = x[i];
        st.s.y = y[i];

        chunk_index[cube->find_chunk_that_contains(st)].push_back(i);
    }

    std::vector<std::vector<double>> out;
    out.resize(cube->bands().count());
    for (uint16_t ib = 0; ib < out.size(); ++ib) {
        out[ib].resize(x.size(), NAN);
    }

    uint32_t nthreads = config::instance()->get_default_chunk_processor()->max_threads();
    std::vector<chunkid_t> chunks;  // vector of keys in chunk_index
    for (auto iter = chunk_index.begin(); iter != chunk_index.end(); ++iter) {
        chunks.push_back(iter->first);
    }

    std::vector<std::thread> workers;
    for (uint32_t ithread = 0; ithread < nthreads; ++ithread) {
        workers.push_back(std::thread([&prg, &cube, &out, &chunk_index, &chunks, &ix, &it, &iy, ithread, nthreads](void) {
            for (uint32_t ic = ithread; ic < chunks.size(); ic += nthreads) {
                try {
                    if (chunks[ic] < cube->count_chunks()) {  // if chunk exists
                        std::shared_ptr<chunk_data> dat = cube->read_chunk(chunks[ic]);
                        if (!dat->empty()) {  // if chunk is not empty
                            // iterate over all query points within the current chunk
                            for (uint32_t i = 0; i < chunk_index[chunks[ic]].size(); ++i) {
                                double ixc = ix[chunk_index[chunks[ic]][i]];
                                double iyc = iy[chunk_index[chunks[ic]][i]];
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

    //
    //    for (auto iter = chunk_index.begin(); iter != chunk_index.end(); ++iter) {
    //        if (iter->first >= cube->count_chunks()) {
    //            continue;
    //        }
    //
    //        std::shared_ptr<chunk_data> dat = cube->read_chunk(iter->first);
    //        if (dat->empty()) {
    //            continue;
    //        }
    //
    //        for (uint32_t i = 0; i < iter->second.size(); ++i) {
    //            double ixc = ix[iter->second[i]];
    //            double iyc = iy[iter->second[i]];
    //            double itc = it[iter->second[i]];
    //
    //            int iix = ((int)std::floor(ixc)) % cube->chunk_size()[2];
    //            int iiy = dat->size()[2] - 1 - (((int)std::floor(iyc)) % cube->chunk_size()[1]);
    //            int iit = ((int)std::floor(itc)) % cube->chunk_size()[0];
    //
    //            // check to prevent out of bounds faults
    //            if (iix < 0 || uint32_t(iix) >= dat->size()[3]) continue;
    //            if (iiy < 0 || uint32_t(iiy) >= dat->size()[2]) continue;
    //            if (iit < 0 || uint32_t(iit) >= dat->size()[1]) continue;
    //
    //            for (uint16_t ib = 0; ib < out.size(); ++ib) {
    //                out[ib][iter->second[i]] = ((double*)dat->buf())[ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + iit * dat->size()[2] * dat->size()[3] + iiy * dat->size()[3] + iix];
    //            }
    //        }
    //    }
    return out;
}

}  // namespace gdalcubes