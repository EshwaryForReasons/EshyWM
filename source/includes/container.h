
#pragma once

#include "util.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <map>

typedef std::unordered_map<Window, std::shared_ptr<class container>> organized_container_map_t;
typedef std::map<int, std::shared_ptr<class container>> ordered_container_map_t;
typedef std::vector<std::shared_ptr<class container>> container_list_t;

enum class EOrientation
{
    O_None,
    O_Horizontal,
    O_Vertical
};

enum class EContainerType
{
    CT_Root,
    CT_Container,
    CT_Leaf
};

class container : public std::enable_shared_from_this<container>
{
public:

    container()
        : orientation(EOrientation::O_Horizontal)
        , container_type(EContainerType::CT_Container)
    {}

    container(EOrientation _orientation)
        : orientation(_orientation)
        , container_type(EContainerType::CT_Container)
    {}

    container(EOrientation _orientation, EContainerType _container_type)
        : orientation(_orientation)
        , container_type(_container_type)
    {}

    void add_internal_container(std::shared_ptr<container> container_to_add);
    void remove_internal_container(std::shared_ptr<container> container_to_remove);
    void move_internal_container(std::shared_ptr<container> container_to_move, int move_amount);

    /**Will only run if container_type == CT_Leaf*/
    std::shared_ptr<class EshyWMWindow> create_window(Window window, XWindowAttributes attr);

    void move_window_horizontal_left_arrow();
    void move_window_horizontal_right_arrow();
    void move_window_vertical_up_arrow();
    void move_window_vertical_down_arrow();

    void set_size(uint width, uint height);
    void set_position(int x, int y);
    void set_size_and_position(int x, int y, uint width, uint height);
    void set_size_and_position(rect size_and_position);

    EOrientation get_orientation() const {return orientation;}
    EContainerType get_container_type() const {return container_type;}
    std::shared_ptr<class EshyWMWindow> get_window() const {return window_internal;}
    container_list_t get_container_list() const {return container_list_internal;}
    rect get_size_and_position() const {return size_and_position_internal;}

    int get_horizontal_position() const {return horizontal_position;}
    int get_vertical_position() const {return vertical_position;}

    /**Only runs if container_type == CT_Root. Recurses through all child containers and returns a vector*/
    container_list_t get_all_container_list();
    organized_container_map_t get_all_container_map_by_frame();
    organized_container_map_t get_all_container_map_by_titlebar();
    ordered_container_map_t get_all_container_map_by_horizontal_position();
    ordered_container_map_t get_all_container_map_by_vertical_position();

private:

    EOrientation orientation;
    EContainerType container_type;

    std::shared_ptr<class EshyWMWindow> window_internal;
    container_list_t container_list_internal;

    rect size_and_position_internal;

    int horizontal_position;
    int vertical_position;

    container_list_t get_all_leaf_container_list();
};