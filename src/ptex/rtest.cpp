#include <string>
#include <alloca.h>
#include <iostream>
#include "Ptexture.h"

void DumpData(Ptex::DataType dt, int nchan, PtexFaceData* dh, std::string prefix)
{
    float* pixel = (float*) alloca(sizeof(float)*nchan);
    uint8_t* cpixel = (uint8_t*) alloca(sizeof(uint8_t)*nchan);
    Ptex::Res res = dh->res();
    std::cout << prefix;
    if (dh->isTiled()) {
	Ptex::Res tileres = dh->tileRes();
	printf("tiled data (%d x %d):\n", tileres.u(), tileres.v());
	int n = res.ntiles(tileres);
	for (int i = 0; i < n; i++) {
	    PtexFaceData* t = dh->getTile(i);
	    std::cout << prefix << i;
	    DumpData(dt, nchan, t, prefix + "  ");
	    t->release();
	}
    } else {
	int ures, vres;
	if (dh->isConstant()) { ures = vres = 1; std::cout << "const data: "; }
	else { ures = res.u(); vres = res.v(); std::cout << "data:\n"; }
	
	int vimax = vres; if (vimax > 16) vimax = 16;
	for (int vi = 0; vi < vimax; vi++) {
	    std::cout << prefix << "  ";
	    int uimax = ures; if (uimax > 16) uimax = 16;
	    for (int ui = 0; ui < uimax; ui++) {
		Ptex::ConvertToFloat(pixel, dh->getPixel(ui, vi), dt, nchan);
		Ptex::ConvertFromFloat(cpixel, pixel, Ptex::dt_uint8, nchan);
		for (int c=0; c < nchan; c++) {
		    printf("%02x", cpixel[c]);
		}
		printf(" ");
	    }
	    if (uimax != ures) printf(" ...");
	    printf("\n");
	}
	if (vimax != vres) std::cout << prefix << "  ..." << std::endl;
    }
}

void DumpMetaData(PtexMetaData* meta)
{
    std::cout << "meta:" << std::endl;
    for (int i = 0; i < meta->numKeys(); i++) {
	const char* key;
	Ptex::MetaDataType type;
	meta->getKey(i, key, type);
	std::cout << "  " << key << " type=" << Ptex::MetaDataTypeName(type);
	int count;
	switch (type) {
	case Ptex::mdt_string:
	    {
		const char* val=0;
		meta->getValue(key, val);
		std::cout <<  "  " << val;
	    }
	    break;
	case Ptex::mdt_int8:
	    {
		const int8_t* val=0;
		meta->getValue(key, val, count);
		for (int j = 0; j < count; j++)
		    std::cout <<  "  " << val[j];
	    }
	    break;
	case Ptex::mdt_int16:
	    {
		const int16_t* val=0;
		meta->getValue(key, val, count);
		for (int j = 0; j < count; j++)
		    std::cout <<  "  " << val[j];
	    }
	    break;
	case Ptex::mdt_int32:
	    {
		const int32_t* val=0;
		meta->getValue(key, val, count);
		for (int j = 0; j < count; j++)
		    std::cout <<  "  " << val[j];
	    } 
	    break;
	case Ptex::mdt_float:
	    {
		const float* val=0;
		meta->getValue(key, val, count);
		for (int j = 0; j < count; j++)
		    std::cout <<  "  " << val[j];
	    }
	    break;
	case Ptex::mdt_double:
	    {
		const double* val=0;
		meta->getValue(key, val, count);
		for (int j = 0; j < count; j++)
		    std::cout <<  "  " << val[j];
	    }
	    break;
	}
	std::cout << std::endl;
    }
}

int main(int argc, char** argv)
{
    int maxmem = argc >= 2 ? atoi(argv[1]) : 0;
    PtexCache* c = PtexCache::create(0, maxmem);

    std::string error;
    PtexTexture* r = c->get("test.ptx", error);

    if (!r) {
	std::cerr << error << std::endl;
	return 1;
    }
    std::cout << "meshType " << Ptex::MeshTypeName(r->meshType()) << std::endl;
    std::cout << "dataType " << Ptex::DataTypeName(r->dataType()) << std::endl;
    std::cout << "alphaChannel " << r->alphaChannel() << std::endl;
    std::cout << "numChannels " << r->numChannels() << std::endl;
    std::cout << "numFaces " << r->numFaces() << std::endl;

    PtexMetaData* meta = r->getMetaData();
    std::cout << "numMeta " << meta->numKeys() << std::endl;
    DumpMetaData(meta);
    meta->release();

    int nfaces = r->numFaces();
    for (int i = 0; i < nfaces; i++) {
	const Ptex::FaceInfo& f = r->getFaceInfo(i);
#if 1
	std::cout << "face " << i << ":\n"
		  << "  res: " << int(f.res.ulog2) << ' ' << int(f.res.vlog2) << "\n"
		  << "  adjface: " 
		  << f.adjfaces[0] << ' '
		  << f.adjfaces[1] << ' '
		  << f.adjfaces[2] << ' '
		  << f.adjfaces[3] << "\n"
		  << "  adjedge: " 
		  << f.adjedge(0) << ' '
		  << f.adjedge(1) << ' '
		  << f.adjedge(2) << ' '
		  << f.adjedge(3) << "\n"
		  << "  flags: " << f.flags << "\n";
#endif

	PtexFaceData* dh = r->getData(i, f.res);
#if 0
	DumpData(r->dataType(), r->numChannels(), dh, "  ");
#else
	{
	    int ures=f.res.u(), vres=f.res.v();
	    Ptex::DataType dt = r->dataType();
	    int nchan = r->numChannels();
	    float* pixel = (float*) alloca(sizeof(float)*nchan);
	    uint8_t* cpixel = (uint8_t*) alloca(sizeof(uint8_t)*nchan);
	    int pixelsize = Ptex::DataSize(dt) * nchan;
	    int rowlen = ures * pixelsize;
	    int stride = rowlen + 64;
	    void* buff = malloc(stride * vres);
	    r->getData(i, buff, stride);
	    std::cout << "data:";
	
	    int vimax = vres; if (vimax > 16) vimax = 16;
	    for (int vi = 0; vi < vimax; vi++) {
		std::cout << "  ";
		int uimax = ures; if (uimax > 16) uimax = 16;
		for (int ui = 0; ui < uimax; ui++) {
#if 0
		    void* src = ((char*)buff) + stride * vi + pixelsize * ui;
#else
		    void* src = dh->getPixel(ui, vi);
#endif
		    Ptex::ConvertToFloat(pixel, src, dt, nchan);
		    Ptex::ConvertFromFloat(cpixel, pixel, Ptex::dt_uint8, nchan);
		    for (int c=0; c < nchan; c++) {
			printf("%02x", cpixel[c]);
		    }
		    printf(" ");
		}
		if (uimax != ures) printf(" ...");
		printf("\n");
	    }
	    if (vimax != vres) std::cout << "  ..." << std::endl;
	}
#endif
	dh->release();
    }

#if 0
#define log(x) x // std::cout << "\n*** " #x "\n" << std::endl; x

    log(c->get("test.ptx", error)->release(););
    log(c->get("test2.ptx", error)->release(););
    log(c->purge("test.ptx"););
    log(c->get("test.ptx", error)->release(););
    log(c->get("test2.ptx", error)->release(););
    log(c->purgeAll(););
    log(c->get("test2.ptx", error)->release(););
    log(c->release(););
    log(r->release(););
#endif
    return 0;
}
