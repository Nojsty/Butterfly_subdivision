#include "application.hpp"
#include <cassert>
#include <unordered_set>

/**
 * Accepts a vertex of the source (coarse) mesh and applies the vertex rule of the Butterfly subdivision to compute a
 * position of a new vertex of the destination (refined) mesh.
 * <p>
 * NOTE: The source mesh uses the "half-edge" representation.
 *
 * @param vertex The vertex to process.
 * @return A position of the new vertex.
 */
glm::vec3 Application::vertex_rule(Vertex const* const vertex) {
    glm::vec3 result = glm::vec3(0, 0, 0);
    {
	// vertex rule doesnt do anything, modified butterfly is an interpolation subdivision algorithm
        result = vertex->position;  
    }

    return result;
}

/**
 * Accepts an edge of the source (coarse) mesh and applies the edge rule of the Butterfly subdivision to compute a position of a
   new vertex of the destination (refined) mesh. 
 * <p>
 * NOTE: The source mesh uses the "half-edge" representation.
 *
 * @param edge The edge to process. From the source mesh.
 * @return A position of the new vertex.
 */
glm::vec3 Application::edge_rule(Edge const* const edge, float const w) {
    std::array<Vertex const*, 8> vertices = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    {
        // collect all 8 vertices referenced by the edge rule of the subdivision scheme
        vertices[0] = edge->start;            
        vertices[1] = edge->end;          
        vertices[2] = edge->next->end;        
        vertices[3] = edge->twin->next->end;
        vertices[4] = edge->next->twin->prev->start;
        vertices[5] = edge->prev->twin->prev->start;
        vertices[6] = edge->twin->prev->twin->prev->start;
        vertices[7] = edge->twin->next->twin->prev->start;
    }

    glm::vec3 result = glm::vec3(0, 0, 0);
    {
	// use the vertices collected above to compute a position of the resulting vertex
        // [0] and [1] or .start and .end of edge have weight = 1/2; [2] and [3] weight = 2w, [4] [5] [6] and [7] weight = -w
        result.x = (float)(vertices[0]->position.x * 0.5 + vertices[1]->position.x * 0.5 + vertices[2]->position.x * 2 * w + vertices[3]->position.x * 2 * w +
                   vertices[4]->position.x * -w + vertices[5]->position.x * -w + vertices[6]->position.x * -w + vertices[7]->position.x * -w);
        result.y = (float)(vertices[0]->position.y * 0.5 + vertices[1]->position.y * 0.5 + vertices[2]->position.y * 2 * w + vertices[3]->position.y * 2 * w +
                   vertices[4]->position.y * -w + vertices[5]->position.y * -w + vertices[6]->position.y * -w + vertices[7]->position.y * -w);
        result.z = (float)(vertices[0]->position.z * 0.5 + vertices[1]->position.z * 0.5 + vertices[2]->position.z * 2 * w + vertices[3]->position.z * 2 * w +
                   vertices[4]->position.z * -w + vertices[5]->position.z * -w + vertices[6]->position.z * -w + vertices[7]->position.z * -w);
    }

    return result;
}

/**
 * Given a source (coarse) mesh "src_mesh" the method applies the Butterfly subdivision scheme in order to obtain the
 * destination (refined) mesh "dst_mesh". The destination mesh "dst_mesh" passed to the method must be empty, and
 * the method then builds the content according to the Butterfly subdivision scheme.
 * <p>
 * NOTE: The source mesh "src_mesh" uses the "half-edge" representation. 
 *       The destination mesh "dst_mesh" must also use the "half-edge" representation.
 *
 * @param 	src_mesh	The source mesh.
 * @param 	dst_mesh	The destination mesh. Initially empty.
 */
void Application::butterfly_subdivision(SubdivisionTriangleMesh const& src_mesh, float const w, SubdivisionTriangleMesh& dst_mesh) {
    SubdivisionTriangleMeshBuilder bld(&dst_mesh);      // call for builder in .hpp

    // iteration over all the triangles of the source mesh
    for (Face const& face : src_mesh.faces) {

	// store the vertices (edges respectivelly) generated by their rules, then insert them to the dst_mesh
        std::vector<Vertex*> vertex_rule_vertices, edge_rule_vertices;  
        {
            // compute subdivision vertices using the rules
            
            // vertex rule vertices
            std::vector<Vertex*> face_vertices;
            face_vertices.push_back( face.edge->start );
            face_vertices.push_back( face.edge->end );
            face_vertices.push_back( face.edge->next->end );

            for ( Vertex* v : face_vertices )
            {
                Vertex * toProcess = bld.find_dst_vertex_of(v);
				
                // vertex has not been computed yet
                if (  toProcess == nullptr )
                {
                    Vertex temp;

                    temp.position =  Application::vertex_rule( v );
                    toProcess = bld.insert_vertex( temp.position, v );

                    vertex_rule_vertices.push_back( toProcess );
                }
                // vertex has already been computed
                else
                {
                   vertex_rule_vertices.push_back( toProcess );
                }
            } 
            
            // edge rule vertices from edges:
            std::vector<Edge*> face_edges;
            face_edges.push_back( face.edge );
            face_edges.push_back( face.edge->next );
            face_edges.push_back( face.edge->prev );

            for ( Edge* e : face_edges )
            {
                Vertex * toProcess = bld.find_dst_vertex_of(e);

                // vertex has not been computed yet
                if (  toProcess == nullptr )
                {
                    Vertex temp;

                    temp.position = Application::edge_rule( e, w );
                    toProcess = bld.insert_vertex( temp.position, e );
                    
                    edge_rule_vertices.push_back( toProcess );
                }
                // vertex has already been computed
                else 
                {
                    edge_rule_vertices.push_back( toProcess );
                }
            }     
        }
		
        // inserting 4 triangles as a result of the subdivision into the builder bld
        bld.insert_triangle( vertex_rule_vertices.at(0), edge_rule_vertices.at(0), edge_rule_vertices.at(2) );
        bld.insert_triangle( edge_rule_vertices.at(0), vertex_rule_vertices.at(1), edge_rule_vertices.at(1) );
        bld.insert_triangle( edge_rule_vertices.at(2), edge_rule_vertices.at(1), vertex_rule_vertices.at(2) );
        bld.insert_triangle( edge_rule_vertices.at(0), edge_rule_vertices.at(1), edge_rule_vertices.at(2) );
    }

    bld.finalize();

    check_mesh_invariants(dst_mesh);
}
