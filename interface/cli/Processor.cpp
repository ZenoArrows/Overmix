/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "Processor.hpp"
#include "Parsing.hpp"

#include "planes/ImageEx.hpp"
#include "color.hpp"

#include <QString>
#include <QDebug>

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace Overmix;


template<typename T>
T getEnum( QString str, std::vector<std::pair<const char*, T>> cases ){
	auto pos = std::find_if( cases.begin(), cases.end(), [&]( auto pair ){ return pair.first == str; } );
	if( pos != cases.end() )
		return pos->second;
	throw std::invalid_argument( "Unknown enum value" );
}

template<typename Arg, typename Arg2, typename... Args>
void convert( QString str, Arg& val, Arg2& val2, Args&... args ){
	Splitter split( str, ':' );
	convert( split.left, val );
	convert( split.right, val2, args... );
}

void convert( QString str, double& val ) { val = asDouble(str); }
void convert( QString str, int& val ) { val = asInt(str); }

template<typename T>
void convert( QString str, Point<T>& val ){
	Splitter split( str, 'x' );
	convert( split.left,  val.x );
	convert( split.right, val.y );
}

void convert( QString str, ScalingFunction& func ){
	func = getEnum<ScalingFunction>( str,
		{	{ "none",     ScalingFunction::SCALE_NEAREST   }
		,	{ "linear",   ScalingFunction::SCALE_LINEAR    }
		,	{ "mitchell", ScalingFunction::SCALE_MITCHELL  }
		,	{ "catrom",   ScalingFunction::SCALE_CATROM    }
		,	{ "spline",   ScalingFunction::SCALE_SPLINE    }
		,	{ "lanczos3", ScalingFunction::SCALE_LANCZOS_3 }
		,	{ "lanczos5", ScalingFunction::SCALE_LANCZOS_5 }
		,	{ "lanczos7", ScalingFunction::SCALE_LANCZOS_7 }
		} );
}

using PlaneFunc = Plane (Plane::*)() const;
void convert( QString str, PlaneFunc& func ){
	func = getEnum<PlaneFunc>( str,
		{	{ "robert",          &Plane::edge_robert          }
		,	{ "sobel",           &Plane::edge_sobel           }
		,	{ "prewitt",         &Plane::edge_prewitt         }
		,	{ "laplacian",       &Plane::edge_laplacian       }
		,	{ "laplacian-large", &Plane::edge_laplacian_large }
		} );
}

struct ScaleProcessor : public Processor{
	ScalingFunction function{ ScalingFunction::SCALE_CATROM };
	Point<double> scale;
	
	ScaleProcessor( QString str ) { convert( str, function, scale ); }
	
	void process( ImageEx& img ) override
		{ img.scaleFactor( scale, function ); }
};

struct EdgeProcessor : public Processor {
	PlaneFunc function;
	
	EdgeProcessor( QString str ) { convert( str, function ); }
	
	void process( ImageEx& img ) override
		{ img.apply( function ); }
};

struct DilateProcessor : public Processor {
	int size;
	
	DilateProcessor( QString str ) { convert( str, size ); }
	
	void process( ImageEx& img ) override
		{ img.apply( &Plane::dilate, size ); }
};

struct BinarizeThresholdProcessor : public Processor {
	double threshold;
	
	BinarizeThresholdProcessor( QString str ) { convert( str, threshold ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i].binarize_threshold( color::fromDouble( threshold ) );
	}
};

struct BinarizeAdaptiveProcessor : public Processor {
	int amount;
	double threshold;
	
	BinarizeAdaptiveProcessor( QString str ) { convert( str, amount, threshold ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i].binarize_adaptive( amount, color::fromDouble( threshold ) );
	}
};

struct BinarizeDitherProcessor : public Processor {
	BinarizeDitherProcessor( QString ) { }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i].binarize_dither();
	}
};

struct BlurProcessor : public Processor {
	Point<double> deviation;
	
	BlurProcessor( QString str ) { convert( str, deviation ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i] = img[i].blur_gaussian( deviation.x, deviation.y );
	}
};

struct DeconvolveProcessor : public Processor {
	double deviation;
	int iterations;
	
	DeconvolveProcessor( QString str ) { convert( str, deviation, iterations ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i] = img[i].deconvolve_rl( deviation, iterations );
	}
};

struct LevelProcessor : public Processor {
	double limit_min, limit_max, output_min, output_max, gamma;
	
	LevelProcessor( QString str )
		{ convert( str, limit_min, limit_max, output_min, output_max, gamma ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i] = img[i].level( 
					color::fromDouble( limit_min )
				,	color::fromDouble( limit_max )
				,	color::fromDouble( output_min )
				,	color::fromDouble( output_max )
				,	gamma
				);
	}
};


std::unique_ptr<Processor> Overmix::processingParser( QString parameters ){
	Splitter split( parameters, ':' );
	if( split.left == "scale" )
		return std::make_unique<ScaleProcessor>( split.right );
	if( split.left == "edge" )
		return std::make_unique<EdgeProcessor>( split.right );
	if( split.left == "dilate" )
		return std::make_unique<DilateProcessor>( split.right );
	if( split.left == "binarize-threshold" )
		return std::make_unique<BinarizeThresholdProcessor>( split.right );
	if( split.left == "binarize-adaptive" )
		return std::make_unique<BinarizeAdaptiveProcessor>( split.right );
	if( split.left == "binarize-dither" )
		return std::make_unique<BinarizeDitherProcessor>( split.right );
	if( split.left == "blur" )
		return std::make_unique<BlurProcessor>( split.right );
	if( split.left == "deconvolve" )
		return std::make_unique<DeconvolveProcessor>( split.right );
	if( split.left == "level" )
		return std::make_unique<LevelProcessor>( split.right );
	qDebug() << "No processor found!" << split.left;
	
	return {};
}