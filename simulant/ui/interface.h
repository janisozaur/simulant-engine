#ifndef INTERFACE_H
#define INTERFACE_H

#include <memory>
#include <unordered_map>
#include <tinyxml.h>

#include "../deps/nuklear/nuklear.h"
#include "../generic/managed.h"
#include"../types.h"
#include "../loadable.h"
#include "../renderers/batching/renderable.h"
#include "../vertex_data.h"
#include "../utils/unicode.h"
#include "../hardware_buffer.h"
#include "element.h"

namespace smlt {

class WindowBase;

namespace ui {

class Interface;


class ElementList {
public:
    ElementList(const std::vector<Element>& elements):
        elements_(elements) {}

    void set_text(const unicode& text) {
        for(Element& e: elements_) {
            e.set_text(text);
        }
    }

    ElementList append_row() {
        std::vector<Element> new_elements;
        for(Element& e: elements_) {
            new_elements.push_back(e.append_row());
        }

        return ElementList(new_elements);
    }

    ElementList append_label(const unicode& text) {
        std::vector<Element> new_elements;
        for(Element& e: elements_) {
            new_elements.push_back(e.append_label(text));
        }

        return ElementList(new_elements);
    }

    ElementList append_progress_bar() {
        std::vector<Element> new_elements;
        for(Element& e: elements_) {
            new_elements.push_back(e.append_progress_bar());
        }

        return ElementList(new_elements);
    }

    bool is(const std::string& selector) {
        if(selector != ":visible") {
            L_WARN("Unsupported selector: " + selector);
            return false;
        }

        for(Element& e: elements_) {
            if(e.is_visible()) {
                return true;
            }
        }

        return false;
    }

    ElementList add_class(const unicode& cl) {
        for(Element& e: elements_) {
            e.add_class(cl);
        }

        return *this;
    }

    ElementList set_event_callback(EventType event_type, EventCallback func) {
        for(Element& e: elements_) {
            e.set_event_callback(event_type, func);
        }

        return *this;
    }

    ElementList remove_class(const unicode& cl) {
        for(Element& e: elements_) {
            e.remove_class(cl);
        }

        return *this;
    }

    void add_css(const std::string& property, const std::string& value) {
        for(Element& e: elements_) {
            e.add_css(property, value);
        }
    }

    void set_attr(const std::string& property, const std::string& value) {
        for(Element& e: elements_) {
            e.set_attr(property, value);
        }
    }

    Element& operator[](uint32_t i) {
        return elements_[i];
    }

    const Element& operator[](uint32_t i) const {
        return elements_[i];
    }

    std::vector<Element>::iterator begin() { return elements_.begin(); }
    std::vector<Element>::iterator end() { return elements_.end(); }

    bool empty() const { return elements_.empty(); }

    void show(const std::string& transition="") {
        for(Element& e: elements_) {
            e.show(transition);
        }
    }

    void hide() {
        for(Element& e: elements_) {
            e.hide();
        }
    }

    void set_id(const std::string& id) {
        for(Element& e: elements_) {
            e.set_id(id);
        }
    }

    void scroll_to_bottom() {
        for(Element& e: elements_) {
            e.scroll_to_bottom();
        }
    }

    void remove_children() {
        for(Element& e: elements_) {
            e.remove_children();
        }
    }

    void html(const unicode& rml) {
        for(Element& e: elements_) {
            e.inner_rml(rml);
        }
    }

private:
    std::vector<Element> elements_;
};

class UIRenderable:
    public Renderable {

public:
    static const VertexSpecification VERTEX_SPECIFICATION;

public:
    UIRenderable(HardwareBuffer::ptr& index_buffer, HardwareBuffer* vertex_buffer, MaterialID material);

    const MeshArrangement arrangement() const override { return MESH_ARRANGEMENT_TRIANGLES; }
    smlt::RenderPriority render_priority() const override { return RENDER_PRIORITY_MAIN; }
    smlt::Mat4 final_transformation() const override { return Mat4(); }
    const MaterialID material_id() const override { return material_id_; }
    const bool is_visible() const override { return true; }
    const AABB transformed_aabb() const { return AABB(); } // Not used
    const AABB aabb() const { return AABB(); } // Not used

    void prepare_buffers() {
        // This is a no-op as we manually change the buffers every frame manually
    }

    HardwareBuffer* vertex_attribute_buffer() const {
        return vertex_buffer_;
    }

    HardwareBuffer* index_buffer() const {
        return index_buffer_.get();
    }

    VertexSpecification vertex_attribute_specification() const {
        return VERTEX_SPECIFICATION;
    }

    std::size_t index_element_count() const {
        return index_buffer()->size() / sizeof(Index);
    }

private:
    friend class Interface;

    HardwareBuffer* vertex_buffer_ = nullptr;
    std::unique_ptr<HardwareBuffer> index_buffer_;

    MaterialID material_id_;
};


class Interface :
    public Managed<Interface>,
    public Loadable {

public:
    Interface(WindowBase& window, Overlay* owner);
    ~Interface();

    uint16_t width() const;
    uint16_t height() const;

    void set_dimensions(uint16_t width, uint16_t height);

    bool init();
    void update(float dt);
    void render(CameraPtr camera, Viewport viewport);

    void add_css(const std::string& property, const std::string& value);

    ElementList append_row();

    ElementList find(const unicode& selectors);
    void set_styles(const std::string& stylesheet_content);
    void load_font(const unicode& ttf_file);

    WindowBase* window() { return &window_; }

private:    
    friend class ElementImpl;

    std::vector<unicode> find_fonts();

    unicode locate_font(const unicode& filename);

    WindowBase& window_;
    Overlay* stage_ = nullptr;

    TiXmlDocument document_;
    std::shared_ptr<Element> root_element_;
    std::unordered_map<TiXmlElement*, std::shared_ptr<ElementImpl>> element_impls_;

    nk_context nk_ctx_;
    nk_panel nk_layout_;
    nk_font_atlas nk_font_;
    struct nk_smlt_device {
        struct nk_buffer cmds;
        struct nk_draw_null_texture null;
        MaterialID font_tex;
        MaterialID null_tex;
    } nk_device_;

    void send_to_renderer(CameraPtr camera, Viewport viewport);

    std::unique_ptr<HardwareBuffer> shared_vertex_buffer_;

};

}
}

#endif // INTERFACE_H
