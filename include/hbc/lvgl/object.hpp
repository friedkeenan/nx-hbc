#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/init.hpp>
#include <hbc/lvgl/color.hpp>
#include <hbc/lvgl/style.hpp>

namespace hbc::gui {

    /* Forward declare. */
    struct theme;

}

namespace hbc::lvgl {

    enum class alignment : std::underlying_type_t<lv_align_t> {
        /* NOTE: We leave out 'LV_ALIGN_DEFAULT'. */

        out_top_left = LV_ALIGN_OUT_TOP_LEFT,
        out_left_top = LV_ALIGN_OUT_LEFT_TOP,
        top_left     = LV_ALIGN_TOP_LEFT,

        out_left_center = LV_ALIGN_OUT_LEFT_MID,
        left_center     = LV_ALIGN_LEFT_MID,

        bottom_left     = LV_ALIGN_BOTTOM_LEFT,
        out_left_bottom = LV_ALIGN_OUT_LEFT_BOTTOM,
        out_bottom_left = LV_ALIGN_OUT_BOTTOM_LEFT,

        out_top_center = LV_ALIGN_OUT_TOP_MID,
        top_center     = LV_ALIGN_TOP_MID,

        center = LV_ALIGN_CENTER,

        bottom_center     = LV_ALIGN_BOTTOM_MID,
        out_bottom_center = LV_ALIGN_OUT_BOTTOM_MID,

        out_top_right = LV_ALIGN_OUT_TOP_RIGHT,
        out_right_top = LV_ALIGN_OUT_RIGHT_TOP,
        top_right     = LV_ALIGN_TOP_RIGHT,

        out_right_center = LV_ALIGN_OUT_RIGHT_MID,
        right_center     = LV_ALIGN_RIGHT_MID,

        bottom_right     = LV_ALIGN_BOTTOM_RIGHT,
        out_right_bottom = LV_ALIGN_OUT_RIGHT_BOTTOM,
        out_bottom_right = LV_ALIGN_OUT_BOTTOM_RIGHT,
    };

    namespace impl {

        constexpr lv_align_t to_lv_align(const lvgl::alignment alignment) {
            return static_cast<lv_align_t>(std::to_underlying(alignment));
        }

    }

    template<typename T>
    requires (std::integral<T> || std::floating_point<T>)
    struct vector_2d {
        T x;
        T y;

        constexpr vector_2d operator +(this const vector_2d self, const vector_2d rhs) {
            return {self.x + rhs.x, self.y + rhs.y};
        }

        constexpr vector_2d operator -(this const vector_2d self, const vector_2d rhs) {
            return {self.x - rhs.x, self.y - rhs.y};
        }
    };

    struct percentage {
        std::int32_t value;

        /* A function to call in constant evaluation to indicate an erroneous value. */
        static void _invalid_percentage_value();

        constexpr explicit(false) operator std::int32_t(this const percentage self) {
            if consteval {
                if (self.value < 0 || self.value > 1000) {
                    _invalid_percentage_value();
                }
            }

            /* A value in a special range for LVGL to consume as a percentage. */
            return LV_PCT(self.value);
        }
    };

    namespace impl {

        struct screen_t {};

        /*
            A dummy object used to signal that an object
            should be constructed as a "screen" object.
        */
        constexpr inline auto screen = impl::screen_t{};

    }

    /* Forward declare. */
    struct object;

    template<typename Parent>
    concept parentable = (
        std::derived_from<Parent, lvgl::object> ||

        std::same_as<Parent, const impl::screen_t>
    );

    /*
        TODO: See if it would make sense to have an 'object_view'
        for objects that don't have a C++ counterpart.

        Would maybe be desirable for other object types as well.
    */

    struct object {
        HBC_NON_COPYABLE(object);

        using event_callback_t = void (*)(lvgl::event);

        lv_obj_t *_object;

        std::vector<std::unique_ptr<object>> _children;

        [[nodiscard]]
        inline lv_obj_t *_release(this object &self) {
            return std::exchange(self._object, nullptr);
        }

        inline void _set_self_ptr(this object &self) {
            lv_obj_set_user_data(self._object, &self);
        }

        inline object(object &&other) noexcept
        :
            _object(other._release()),
            _children(std::move(other._children))
        {
            this->_set_self_ptr();
        }

        inline void _delete(this object &self) {
            if (impl::already_destroyed()) {
                return;
            }

            /* Destroy our children before destroying ourselves. */
            self.clear();

            if (self._object != nullptr) {
                lv_obj_delete(self._object);
            }
        }

        inline object &operator =(this object &self, object &&other) noexcept {
            self._delete();

            self._object   = other._release();
            self._children = std::move(other._children);

            self._set_self_ptr();

            return self;
        }

        inline virtual ~object() {
            this->_delete();
        }

        template<typename Factory, typename... Args>
        requires (std::invocable<Factory, lv_obj_t * const &, Args...>)
        object(lvgl::parentable auto &parent, Factory &&factory, Args &&... args) {
            const auto parent_object = [&]() -> lv_obj_t * {
                if constexpr (std::same_as<decltype(parent), const impl::screen_t &>) {
                    return nullptr;
                } else {
                    return parent._object;
                }
            }();

            this->_object = std::invoke(
                std::forward<Factory>(factory),
                parent_object,
                std::forward<Args>(args)...
            );

            this->_set_self_ptr();

            /* By default, don't have any objects be scrollable. */
            this->disable_scrolling();
        }

        object(lvgl::parentable auto &parent) : object(parent, lv_obj_create) {}

        inline lv_obj_t *underlying(this object &self) {
            return self._object;
        }

        inline const lv_obj_t *underlying(this const object &self) {
            return self._object;
        }

        static inline object &from_underlying(lv_obj_t *underlying) {
            return *static_cast<object *>(lv_obj_get_user_data(underlying));
        }

        template<std::derived_from<object> Child, typename... Args>
        requires (std::constructible_from<Child, object &, Args...>)
        Child &add_child(this object &self, Args &&... args) {
            /* NOTE: Will immediately get passed to the 'unique_ptr' constructor. */
            const auto child_ptr = new Child(
                self,
                std::forward<Args>(args)...
            );

            self._children.emplace_back(child_ptr);

            return *child_ptr;
        }

        template<std::derived_from<object> Parent = object>
        Parent &parent(this object &self) {
            const auto parent = lv_obj_get_parent(self._object);

            return static_cast<Parent &>(from_underlying(parent));
        }

        template<std::derived_from<object> Parent = object>
        Parent &nth_parent(this object &self, const std::size_t n) {
            auto parent = lv_obj_get_parent(self._object);
            for (auto _ : std::views::iota(0uz, n)) {
                parent = lv_obj_get_parent(parent);
            }

            return static_cast<Parent &>(from_underlying(parent));
        }

        void remove_from_parent(this object &self) {
            auto &parent = self.parent();

            for (const auto it : std::views::iota(parent._children.begin(), parent._children.end())) {
                if (it->get() == &self) {
                    parent._children.erase(it);

                    return;
                }
            }
        }

        void remove_branch_from_nth_parent(this object &self, const std::size_t n) {
            auto branch = self._object;
            for (auto _ : std::views::iota(0uz, n)) {
                branch = lv_obj_get_parent(branch);
            }

            auto &parent = from_underlying(lv_obj_get_parent(branch));

            for (const auto it : std::views::iota(parent._children.begin(), parent._children.end())) {
                if (it->get()->_object == branch) {
                    parent._children.erase(it);

                    return;
                }
            }
        }

        template<std::derived_from<object> Child = object>
        Child &child(this object &self, const std::size_t index) {
            auto &child = *self._children[index];

            return static_cast<Child &>(child);
        }

        inline bool has_children(this const object &self) {
            return !self._children.empty();
        }

        inline std::size_t num_children(this const object &self) {
            return self._children.size();
        }

        template<std::derived_from<object> Child = object>
        Child &last_child(this object &self) {
            auto &child = *self._children.back();

            return static_cast<Child &>(child);
        }

        template<std::derived_from<object> Child = object>
        Child &first_child(this object &self) {
            auto &child = *self._children.front();

            return static_cast<Child &>(child);
        }

        inline void clear_after_index(this object &self, const std::size_t index) {
            self._children.erase(
                std::ranges::next(self._children.begin(), index + 1, self._children.end()),

                self._children.end()
            );
        }

        inline void clear(this object &self) {
            self._children.clear();
        }

        static inline void _event_callback_shim(lv_event_t *event) {
            /*
                NOTE: This is well-defined since our C++ implementation supports
                converting from 'void *' to function pointers and vice versa,
                and thus the round-trip conversion must yield the original value.
            */
            const auto real_callback = reinterpret_cast<object::event_callback_t>(
                lv_event_get_user_data(event)
            );

            real_callback(lvgl::event{event});
        }

        inline void add_event_callback(this object &self, const lv_event_code_t code, const object::event_callback_t callback) {
            lv_obj_add_event_cb(self._object, object::_event_callback_shim, code, reinterpret_cast<void *>(callback));
        }

        inline void on_pressed(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_PRESSED, callback);
        }

        inline void on_released(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_RELEASED, callback);
        }

        inline void on_clicked(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_CLICKED, callback);
        }

        inline void on_short_clicked(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_SHORT_CLICKED, callback);
        }

        inline void on_single_clicked(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_SINGLE_CLICKED, callback);
        }

        inline void on_double_clicked(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_DOUBLE_CLICKED, callback);
        }

        inline void on_hover_over(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_HOVER_OVER, callback);
        }

        inline void on_hover_leave(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_HOVER_LEAVE, callback);
        }

        inline void on_size_changed(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_SIZE_CHANGED, callback);
        }

        inline void on_style_changed(this object &self, const object::event_callback_t callback) {
            self.add_event_callback(LV_EVENT_STYLE_CHANGED, callback);
        }

        inline void add_style(this object &self, const lvgl::style &style, const lv_style_selector_t selector = 0) {
            lv_obj_add_style(self._object, &style.underlying(), selector);
        }

        inline void remove_flag(this object &self, const lv_obj_flag_t flag) {
            lv_obj_remove_flag(self._object, flag);
        }

        inline void add_flag(this object &self, const lv_obj_flag_t flag) {
            lv_obj_add_flag(self._object, flag);
        }

        inline void update_flag(this object &self, const lv_obj_flag_t flag, const bool value) {
            lv_obj_update_flag(self._object, flag, value);
        }

        inline void disable_scrolling(this object &self) {
            self.remove_flag(LV_OBJ_FLAG_SCROLLABLE);
        }

        inline void enable_scrolling(this object &self) {
            self.add_flag(LV_OBJ_FLAG_SCROLLABLE);
        }

        inline void set_scroll_direction(this object &self, const lv_dir_t direction) {
            lv_obj_set_scroll_dir(self._object, direction);
        }

        inline void make_floating(this object &self) {
            self.add_flag(LV_OBJ_FLAG_FLOATING);
        }

        inline bool has_state(this const object &self, const lv_state_t state) {
            return lv_obj_has_state(self._object, state);
        }

        inline void add_state(this object &self, const lv_state_t state) {
            lv_obj_add_state(self._object, state);
        }

        inline void remove_state(this object &self, const lv_state_t state) {
            lv_obj_remove_state(self._object, state);
        }

        inline bool is_pressed(this const object &self) {
            return self.has_state(LV_STATE_PRESSED);
        }

        inline bool is_hovered(this const object &self) {
            return self.has_state(LV_STATE_HOVERED);
        }

        inline void set_clickable(this object &self, const bool value) {
            self.update_flag(LV_OBJ_FLAG_CLICKABLE, value);
        }

        inline void set_pos(this object &self, const lvgl::vector_2d<std::int32_t> pos) {
            lv_obj_set_pos(self._object, pos.x, pos.y);
        }

        inline std::int32_t x(this const object &self) {
            return lv_obj_get_x(self._object);
        }

        inline std::int32_t y(this const object &self) {
            return lv_obj_get_y(self._object);
        }

        inline lvgl::vector_2d<std::int32_t> pos(this const object &self) {
            return {self.x(), self.y()};
        }

        inline void refresh_pos(this object &self) {
            lv_obj_refr_pos(self._object);
        }

        inline void set_width(this object &self, const std::int32_t width) {
            lv_obj_set_width(self._object, width);
        }

        inline void set_height(this object &self, const std::int32_t height) {
            lv_obj_set_height(self._object, height);
        }

        inline std::int32_t width(this const object &self) {
            return lv_obj_get_width(self._object);
        }

        inline std::int32_t height(this const object &self) {
            return lv_obj_get_height(self._object);
        }

        inline void set_size(this object &self, const lvgl::vector_2d<std::int32_t> size) {
            lv_obj_set_size(self._object, size.x, size.y);
        }

        inline lvgl::vector_2d<std::int32_t> size(this const object &self) {
            return {self.width(), self.height()};
        }

        inline void refresh_size(this object &self) {
            lv_obj_refr_size(self._object);
        }

        inline void center(this object &self) {
            lv_obj_center(self._object);
        }

        inline void align(this object &self, const lvgl::alignment alignment, const lvgl::vector_2d<std::int32_t> offset = {}) {
            lv_obj_align(self._object, impl::to_lv_align(alignment), offset.x, offset.y);
        }

        inline void align_to(
            this object &self,

            const object &base,
            const lvgl::alignment alignment,
            const lvgl::vector_2d<std::int32_t> offset = {}
        ) {
            lv_obj_align_to(self._object, base._object, impl::to_lv_align(alignment), offset.x, offset.y);
        }

        inline void update_layout(this object &self) {
            lv_obj_update_layout(self._object);
        }

        inline void focus(this object &self) {
            lv_group_focus_obj(self._object);
        }

        inline void invalidate(this object &self) {
            lv_obj_invalidate(self._object);
        }

        inline void set_max_width(this object &self, const std::int32_t width, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_max_width(self._object, width, selector);
        }

        inline void set_max_height(this object &self, const std::int32_t height, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_max_height(self._object, height, selector);
        }

        inline void set_radius(this object &self, const std::int32_t radius, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_radius(self._object, radius, selector);
        }

        inline void set_background_color(this object &self, const lvgl::color color, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_bg_color(self._object, color.underlying(), selector);
        }

        inline void set_background_opacity(this object &self, const lv_opa_t opacity, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_bg_opa(self._object, opacity, selector);
        }

        inline void set_text_color(this object &self, const lvgl::color color, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_text_color(self._object, color.underlying(), selector);
        }

        inline void set_text_align(this object &self, const lv_text_align_t align, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_text_align(self._object, align, selector);
        }

        inline void set_border_width(this object &self, const std::int32_t width, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_border_width(self._object, width, selector);
        }

        inline void set_padding(this object &self, const std::int32_t padding, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_pad_all(self._object, padding, selector);
        }

        inline void set_horizontal_padding(this object &self, const std::int32_t padding, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_pad_hor(self._object, padding, selector);
        }

        inline void set_vertical_padding(this object &self, const std::int32_t padding, const lv_style_selector_t selector = 0) {
            lv_obj_set_style_pad_ver(self._object, padding, selector);
        }

        /*
            Our own custom methods to aid with
            refreshing objects after a theme reloads.

            There's maybe a world where we instead package
            all theme properties into an LVGL style and just
            refresh that but I feel iffy about that.
        */
        virtual inline void on_theme_reload(const gui::theme &) {
            /* Stub implementation. */
        }

        inline void signal_theme_reloaded(this object &self, const gui::theme &theme) {
            self.on_theme_reload(theme);

            for (auto &child : self._children) {
                child->signal_theme_reloaded(theme);
            }
        }
    };

    template<typename Object, typename... Args>
    concept child_constructible_from = requires(lvgl::object &object) {
        object.add_child<Object>(std::declval<Args>()...);
    };

}
