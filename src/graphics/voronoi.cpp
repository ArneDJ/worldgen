#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#define JC_VORONOI_IMPLEMENTATION
#include "../external/voronoi/jc_voronoi.h"

#include "imp.h"
#include "voronoi.h"

#define SEA_LEVEL 0.5
#define MOUNTAIN_LEVEL 0.7
#define NSITES 256*256
#define MAX_SITES 1000

void relax_points(const jcv_diagram* diagram, std::vector<jcv_point> &points)
{
	const jcv_site* sites = jcv_diagram_get_sites(diagram);
	for( int i = 0; i < diagram->numsites; ++i ) {
		const jcv_site* site = &sites[i];
		jcv_point sum = site->p;
		int count = 1;

		const jcv_graphedge* edge = site->edges;

		while (edge) {
			sum.x += edge->pos[0].x;
			sum.y += edge->pos[0].y;
			++count;
			edge = edge->next;
		}

		jcv_point point;
		point.x = sum.x / count;
		point.y = sum.y / count;
		points.push_back(point);
	}
}
/*
struct vcell {
	glm::vec2 center;
	std::vector<vcell*> neighbors;
	std::vector<glm::vec4> borders;
};
*/

struct mycell {
	glm::vec2 center;
	std::vector<glm::vec4> borders;
	std::vector<jcv_site*> neighbors;
};

void gen_cells(struct byteimage *image)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, image->width);

	std::vector<jcv_point> points;

	for (int i = 0; i < MAX_SITES; i++) {
		jcv_point point;
		point.x = dis(gen);
		point.y = dis(gen);
		int x = point.x;
		int y = point.y;
		points.push_back(point);
	}

	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(points.size(), points.data(), 0, 0, &diagram);

	std::vector<jcv_point> relaxed_points;
	relax_points(&diagram, relaxed_points);

	jcv_diagram_generate(relaxed_points.size(), relaxed_points.data(), 0, 0, &diagram);

	//std::vector<struct vcell> cells;
	std::vector<struct mycell> cells;
	// get each cell
	const jcv_site *sites = jcv_diagram_get_sites(&diagram);
	for (int i = 0; i < diagram.numsites; i++) {
		const jcv_site *site = &sites[i];
		struct mycell cell;
		jcv_point p = site->p;
		//struct vcell cell;
		cell.center = glm::vec2(p.x, p.y);
		cells.push_back(cell);
	}

	// get neighbor cells
	for (int i = 0; i < diagram.numsites; i++) { 
		const jcv_site *site = &sites[i];
		jcv_graphedge *edge = site->edges;
		while (edge) {
			cells[i].borders.push_back(glm::vec4(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y));
			jcv_site *neighbor = edge->neighbor;
			if (neighbor != NULL) {
				int index = neighbor->index;
				//printf("%d\n", index);
				cells[i].neighbors.push_back(neighbor);
			}
			edge = edge->next;
		}
	}

	unsigned char sitecolor[3] = {255, 255, 255};
	//vcell cell = cells[10];
	mycell cell = cells[500];
	plot(int(cell.center.x), int(cell.center.y), image->data, image->width, image->height, image->nchannels, sitecolor);
	for (auto &neighbor : cell.neighbors) {
		unsigned char rcolor[3];
		unsigned char basecolor = 100;
		rcolor[0] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[1] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[2] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		const jcv_graphedge *edge = neighbor->edges;
		while( edge ) {
			draw_triangle(neighbor->p.x, neighbor->p.y, edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, image->data, image->width, image->height, image->nchannels, rcolor);
			edge = edge->next;
		}
		/*
		for (auto &border : neighbor->borders) {
			draw_triangle(neighbor->center.x, neighbor->center.y, border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, rcolor);

		}

		*/
	}
	/*
	unsigned char linecolor[3] = {255, 255, 255};
	for (auto border : cell.borders) {
		draw_line(border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, linecolor);

	}
	*/
	// If all you need are the edges
	/*
	const jcv_edge* edge = jcv_diagram_get_edges(&diagram);
	while( edge ) {
		draw_line(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, image->data, image->width, image->height, image->nchannels, linecolor);
		edge = jcv_diagram_get_next_edge(edge);
	}
	*/

}

void do_voronoi(struct byteimage *image, const struct byteimage *heightimage)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, image->width);

	std::vector<jcv_point> points;

	const float ratio = float(image->width) / float(heightimage->width);
	for (int i = 0; i < NSITES; i++) {
		jcv_point point;
		point.x = dis(gen);
		point.y = dis(gen);
		int x = point.x / ratio;
		int y = point.y / ratio;
		points.push_back(point);
	}

	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(points.size(), points.data(), 0, 0, &diagram);

	std::vector<jcv_point> relaxed_points;
	relax_points(&diagram, relaxed_points);

	jcv_diagram_generate(relaxed_points.size(), relaxed_points.data(), 0, 0, &diagram);

	// fill the cells
	const jcv_site *cells = jcv_diagram_get_sites(&diagram);
	for (int i = 0; i < diagram.numsites; i++) {
		unsigned char rcolor[3];
		unsigned char basecolor = 100;
		rcolor[0] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[1] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[2] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		const jcv_site *site = &cells[i];
		const jcv_graphedge *e = site->edges;
		const jcv_point p = site->p;
		int x = p.x / ratio;
		int y = p.y / ratio;

		while (e) {
			draw_triangle(site->p.x, site->p.y, e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y, image->data, image->width, image->height, image->nchannels, rcolor);
			e = e->next;
		}
	}

	unsigned char linecolor[3] = {0, 0, 0};
	// If all you need are the edges
	const jcv_edge* edge = jcv_diagram_get_edges(&diagram);
	while( edge ) {
		draw_line(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, image->data, image->width, image->height, image->nchannels, linecolor);
		edge = jcv_diagram_get_next_edge(edge);
	}

	// plot the sites
	unsigned char sitecolor[3] = {0, 0, 0};
	const jcv_site *sites = jcv_diagram_get_sites(&diagram);
	for (int i = 0; i < diagram.numsites; i++) {
		const jcv_site *site = &sites[i];
		jcv_point p = site->p;
		plot((int)p.x, (int)p.y, image->data, image->width, image->height, image->nchannels, sitecolor);
	}

	jcv_diagram_free(&diagram);
}
