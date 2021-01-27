//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    
    // x 代表当前要渲染的交点， inter 代表 采样光源的交点
    // pdf_light 存储采样光源的概率密度


    // TO DO Implement Path Tracing Algorithm here
    Intersection inter = intersect(ray);

    //若没有相交或在深度过深， 直接返回0.
    if (!inter.happened || depth > MAX_DEPTH)
    {
        return Vector3f{0, 0, 0};
    }

    //发光物体直接返回
    if (inter.m->hasEmission())
    {
        return inter.m->getEmission();
    }
    
    depth++;
    
    // x 代表当前着色点
    Intersection x = inter;
    float pdf_light = 1.0;

    //inter 结果为光源上的点。
    sampleLight(inter, pdf_light);

    Vector3f L_dir = 0, L_indir = 0;
    
    //传入交点到光源的向量 作为光线求交点到光源是否有遮挡, ws 用来表示光源方向
    Vector3f ws = (inter.coords - x.coords).normalized();
    Intersection block = intersect(Ray(x.coords, ws));

    //如果没有发生遮挡,注意下方判断遮挡的方式， 不是用两者的distance， 要用距离判断， 由于光源上的交点采样时没有更新distance
    if (block.distance - (inter.coords - x.coords).norm() > -0.001)
    {   
        //取光源上的法线
        Vector3f NN = inter.normal;
        float r2 = dotProduct(inter.coords - x.coords, inter.coords - x.coords);
        L_dir = inter.emit * x.m->eval(ws, -ray.direction, x.normal) * std::max(0.0f, dotProduct(ws, x.normal))
         * std::max(0.0f, dotProduct(-ws, NN)) / r2 / pdf_light;
    }
    
    //俄罗斯轮盘赌
    if (get_random_float() > RussianRoulette)
    {
        return L_dir;
    }
    Vector3f wi = x.m->sample(-ray.direction, x.normal).normalized();
    Intersection hit = intersect(Ray(x.coords, wi));
    if (x.m->pdf(-ray.direction, wi, x.normal) > EPSILON)
    {
        if (hit.happened && !hit.m->hasEmission())
        {
        
            L_indir = castRay(Ray(x.coords, wi), depth) * x.m->eval(wi, -ray.direction, x.normal) 
            * std::max(0.0f, dotProduct(wi, x.normal)) / x.m->pdf(-ray.direction, wi, x.normal) / RussianRoulette;
        }
    
    }
    

    return L_dir + L_indir;

}


//下方废弃
// Vector3f Scene::shade(Intersection& hit_obj, Vector3f wo) const
// {
//     if (hit_obj.m->hasEmission())
//     {
//         return hit_obj.m->getEmission();
//     }
//     const float epsilon = 0.0005f;
//     // 直接光照贡献
//     Vector3f Lo_dir;
//     {
// 		float light_pdf;
// 		Intersection hit_light;
// 		sampleLight(hit_light, light_pdf);
// 		Vector3f obj2Light = hit_light.coords - hit_obj.coords;
//         Vector3f obj2LightDir = obj2Light.normalized();
       
//         // 检查光线是否被遮挡
//         auto t = intersect(Ray(hit_obj.coords, obj2LightDir));
//         if (t.distance - obj2Light.norm() > -epsilon)
//         {
// 			Vector3f f_r = hit_obj.m->eval(obj2LightDir, wo, hit_obj.normal);
// 			float r2 = dotProduct(obj2Light, obj2Light);
//             float cosA = std::max(.0f, dotProduct(hit_obj.normal,obj2LightDir));
//             float cosB = std::max(.0f, dotProduct(hit_light.normal,-obj2LightDir));
// 			Lo_dir = hit_light.emit * f_r * cosA * cosB / r2 / light_pdf;
//         }
//     }

//     // 间接光照贡献
//     Vector3f Lo_indir;
//     {
// 		if (get_random_float() < RussianRoulette)
// 		{
// 			Vector3f dir2NextObj = hit_obj.m->sample(wo, hit_obj.normal).normalized();
//             float pdf = hit_obj.m->pdf(wo, dir2NextObj, hit_obj.normal);
//             if (pdf > epsilon)
//             {
//                 Intersection nextObj = intersect(Ray(hit_obj.coords, dir2NextObj));
// 				if (nextObj.happened && !nextObj.m->hasEmission())
// 				{
// 					Vector3f f_r = hit_obj.m->eval(dir2NextObj, wo, hit_obj.normal); //BRDF
// 					float cos = std::max(.0f, dotProduct(dir2NextObj, hit_obj.normal));
// 					Lo_indir = shade(nextObj, -dir2NextObj) * f_r * cos / pdf / RussianRoulette;
// 				}
//             }
// 		}
//     }

//     return Lo_dir + Lo_indir;
// }
