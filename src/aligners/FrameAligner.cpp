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


#include "FrameAligner.hpp"
#include "RecursiveAligner.hpp"
#include "../renders/FloatRender.hpp"
#include "../containers/ImageContainer.hpp"
#include "../containers/FrameContainer.hpp"

void FrameAligner::align( AProcessWatcher* watcher ){
	auto frames = getFrames();
	auto base_point = minPoint();
	
	ImageContainer images;
	for( auto& frame : frames ){
		FrameContainer current( *this, frame );
		images.addImage( FloatRender( 1.0, 1.0 ).render( current ) );
		//TODO: this should be a sub-pixel precision render!
	}
	
	RecursiveAligner aligner( images, get_method(), get_scale() ); //TODO: try with AverageAligner
	aligner.addImages();
	aligner.align();
	
	for( unsigned i=0; i<frames.size(); ++i ){
		FrameContainer current( *this, frames[i] );
		auto aligned_offset = base_point - current.minPoint();
		current.offsetAll( aligned_offset + (images.pos(i) - images.minPoint()) );
	}
}

