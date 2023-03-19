
#include "container.h"
#include "window.h"
#include "eshywm.h"
#include "window_manager.h"

#include <algorithm>

void container::add_internal_container(std::shared_ptr<container> container_to_add)
{
    container_list_internal.push_back(container_to_add);
}

void container::remove_internal_container(std::shared_ptr<container> container_to_remove)
{
    for(int i = 0; i < container_list_internal.size(); i++)
    {
        if(container_list_internal[i] == container_to_remove)
        {
            container_list_internal.erase(container_list_internal.begin() + i);
            break;
        }
    }
}

void container::move_internal_container(std::shared_ptr<container> container_to_move, int move_amount)
{
    container_list_t::iterator i = std::find(container_list_internal.begin(), container_list_internal.end(), container_to_move);

    if(i != container_list_internal.cend())
    {
        const int old_index = std::distance(container_list_internal.begin(), i);
        i = (i + move_amount == container_list_internal.cend()) ? container_list_internal.begin() : (i == container_list_internal.begin() && move_amount < 0) ? container_list_internal.end() - 1 : i + move_amount;
        const int new_index = std::distance(container_list_internal.begin(), i);

        if (old_index > new_index)
        {
            std::rotate(container_list_internal.rend() - old_index - 1, container_list_internal.rend() - old_index, container_list_internal.rend() - new_index);
        }
        else
        {
            std::rotate(container_list_internal.begin() + old_index, container_list_internal.begin() + old_index + 1, container_list_internal.begin() + new_index + 1);
        }
    }
}


std::shared_ptr<class EshyWMWindow> container::create_window(Window window, XWindowAttributes attr)
{
    if(container_type != EContainerType::CT_Leaf || window_internal)
    {
        return nullptr;
    }

    window_internal = std::make_shared<EshyWMWindow>(window);
    window_internal->frame_window(attr);
    window_internal->setup_grab_events();

    return window_internal;
}


void container::move_window_horizontal_left_arrow()
{
    horizontal_position = std::max(0, horizontal_position--);
    //EshyWM::get_window_manager()->container_size_updated(shared_from_this());
}

void container::move_window_horizontal_right_arrow()
{
    horizontal_position++;
    //EshyWM::get_window_manager()->container_size_updated(shared_from_this());
}

void container::move_window_vertical_up_arrow()
{
    vertical_position = std::max(0, vertical_position--);
    //EshyWM::get_window_manager()->container_size_updated(shared_from_this());
}

void container::move_window_vertical_down_arrow()
{
    vertical_position--;
    //EshyWM::get_window_manager()->container_size_updated(shared_from_this());
}


container_list_t container::get_all_container_list()
{
    if(container_type != EContainerType::CT_Root)
    {
        return {};
    }

    container_list_t list;
    for (size_t i = 0; i < get_container_list().size(); i++)
    {
        list.push_back(get_container_list()[i]);

        if(get_container_list()[i]->get_container_type() != EContainerType::CT_Leaf)
        {
            const container_list_t recursive_list = get_container_list()[i]->get_all_container_list();
            list.insert(list.end(), recursive_list.begin(), recursive_list.end());
        }
    }

    return list;
}

container_list_t container::get_all_leaf_container_list()
{
    if(container_type != EContainerType::CT_Root)
    {
        return {};
    }

    container_list_t list;
    for (size_t i = 0; i < get_container_list().size(); i++)
    {
        if(get_container_list()[i]->get_container_type() == EContainerType::CT_Leaf)
        {
            list.push_back(get_container_list()[i]);
        }
        else
        {
            const container_list_t recursive_list = get_container_list()[i]->get_all_leaf_container_list();
            list.insert(list.end(), recursive_list.begin(), recursive_list.end());
        }
    }

    return list;
}

organized_container_map_t container::get_all_container_map_by_frame()
{
    if(container_type != EContainerType::CT_Root)
    {
        return {};
    }

    const container_list_t leaf_container_list = get_all_leaf_container_list();

    std::unordered_map<Window, std::shared_ptr<container>> map;
    for (std::shared_ptr<container> c : leaf_container_list)
    {
        map.emplace(c->get_window()->get_frame(), c);
    }

    return map;
}

organized_container_map_t container::get_all_container_map_by_titlebar()
{
    if(container_type != EContainerType::CT_Root)
    {
        return {};
    }

    const container_list_t leaf_container_list = get_all_leaf_container_list();

    std::unordered_map<Window, std::shared_ptr<container>> map;
    for (std::shared_ptr<container> c : leaf_container_list)
    {
        map.emplace(c->get_window()->get_titlebar(), c);
    }

    return map;
}

ordered_container_map_t container::get_all_container_map_by_horizontal_position()
{
    if(container_type != EContainerType::CT_Root)
    {
        return {};
    }

    const container_list_t leaf_container_list = get_all_leaf_container_list();

    ordered_container_map_t map;
    for (std::shared_ptr<container> c : leaf_container_list)
    {
        map.emplace(c->get_horizontal_position(), c);
    }

    return map;
}

ordered_container_map_t container::get_all_container_map_by_vertical_position()
{
    if(container_type != EContainerType::CT_Root)
    {
        return {};
    }

    const container_list_t leaf_container_list = get_all_leaf_container_list();

    ordered_container_map_t map;
    for (std::shared_ptr<container> c : leaf_container_list)
    {
        map.emplace(c->get_vertical_position(), c);
    }

    return map;
}


void container::set_size(uint width, uint height)
{
    size_and_position_internal.width = std::max(width, (uint)1);
    size_and_position_internal.height = std::max(height, (uint)1);
}

void container::set_position(int x, int y)
{
    size_and_position_internal.x = std::max(x, 1);
    size_and_position_internal.y = std::max(y, 1);
}

void container::set_size_and_position(int x, int y, uint width, uint height)
{
    size_and_position_internal.x = std::max(x, 1);
    size_and_position_internal.y = std::max(y, 1);
    size_and_position_internal.width = std::max(width, (uint)1);
    size_and_position_internal.height = std::max(height, (uint)1);
}

void container::set_size_and_position(rect size_and_position)
{
    size_and_position_internal.x = size_and_position.x;
    size_and_position_internal.y = size_and_position.y;
    size_and_position_internal.width = std::max(size_and_position.width, (uint)1);
    size_and_position_internal.height = std::max(size_and_position.height, (uint)1);
}
