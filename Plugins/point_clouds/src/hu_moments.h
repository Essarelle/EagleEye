#pragma once
#include "point_cloudsExport.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <cereal/cereal.hpp>
namespace point_clouds
{
class point_clouds_EXPORT Moment
{
public:
    Moment(float Px_ = 0.0f, float Py_ = 0.0f, float Pz_ = 0.0f);
    template<typename AR>
    void serialize(AR& ar);
    float Evaluate(cv::Mat mask, cv::Mat points, cv::Vec3f centroid);
    float Px, Py, Pz;
};
template<typename AR>
void Moment::serialize(AR& ar)
{
    ar(CEREAL_NVP(Px));
    ar(CEREAL_NVP(Py));
    ar(CEREAL_NVP(Pz));
}
}
