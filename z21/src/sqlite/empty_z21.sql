BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "traction_list" (
	"id"	INTEGER NOT NULL,
	"loco_id"	INTEGER,
	"regulation_step"	INTEGER,
	"time"	REAL,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "update_history" (
	"id"	INTEGER,
	"os"	TEXT,
	"update_date"	TEXT,
	"build_version"	TEXT,
	"build_number"	INTEGER,
	"to_database_version"	INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "layout_data" (
	"id"	INTEGER NOT NULL,
	"name"	TEXT,
	"control_station_type"	TEXT DEFAULT 'free',
	"control_station_theme"	TEXT DEFAULT 'free',
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "vehicles" (
	"id"	INTEGER NOT NULL,
	"name"	TEXT,
	"image_name"	TEXT,
	"type"	INTEGER,
	"max_speed"	INTEGER,
	"address"	INTEGER,
	"active"	INTEGER,
	"position"	INTEGER,
	"drivers_cab"	TEXT,
	"full_name"	TEXT,
	"speed_display"	INTEGER,
	"railway"	TEXT,
	"buffer_lenght"	TEXT,
	"model_buffer_lenght"	TEXT,
	"service_weight"	TEXT,
	"model_weight"	TEXT,
	"rmin"	TEXT,
	"article_number"	TEXT,
	"decoder_type"	TEXT,
	"owner"	TEXT,
	"build_year"	TEXT,
	"owning_since"	TEXT,
	"traction_direction"	INTEGER,
	"description"	TEXT,
	"dummy"	INTEGER,
	"ip"	TEXT,
	"video"	INTEGER,
	"video_x"	INTEGER,
	"video_y"	INTEGER,
	"video_width"	INTEGER,
	"panorama_x"	INTEGER,
	"panorama_y"	INTEGER,
	"panorama_width"	INTEGER,
	"panoramaImage"	TEXT,
	"direct_steering"	INTEGER,
	"crane"	INTEGER DEFAULT 0,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "train_list" (
	"id"	INTEGER NOT NULL,
	"train_id"	INTEGER,
	"vehicle_id"	INTEGER,
	"position"	INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "functions" (
	"id"	INTEGER NOT NULL,
	"vehicle_id"	INTEGER,
	"button_type"	INTEGER NOT NULL DEFAULT 0,
	"shortcut"	TEXT NOT NULL DEFAULT '',
	"time"	TEXT,
	"position"	INTEGER,
	"image_name"	TEXT,
	"function"	INTEGER,
	"show_function_number"	INTEGER NOT NULL DEFAULT 1,
	"is_configured"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "dc_functions" (
	"id"	INTEGER,
	"vehicle_id"	INTEGER,
	"position"	INTEGER,
	"time"	TEXT,
	"image_name"	TEXT,
	"function"	INTEGER,
	"cab_function_description"	TEXT,
	"drivers_cab"	TEXT,
	"shortcut"	TEXT NOT NULL DEFAULT '',
	"button_type"	INT NOT NULL DEFAULT 0,
	"show_function_number"	INTEGER NOT NULL DEFAULT 1,
	"is_configured"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_pages" (
	"id"	INTEGER,
	"position"	INTEGER,
	"name"	TEXT,
	"thumb"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_controls" (
	"id"	INTEGER,
	"page_id"	INTEGER,
	"x"	INTEGER,
	"y"	INTEGER,
	"angle"	REAL,
	"type"	INTEGER,
	"address1"	INTEGER,
	"address2"	INTEGER,
	"address3"	INTEGER,
	"button_type"	INTEGER DEFAULT 0,
	"time"	REAL DEFAULT 0,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_control_states" (
	"id"	INTEGER,
	"control_id"	INTEGER,
	"state"	INTEGER,
	"address1_value"	INTEGER,
	"address2_value"	INTEGER,
	"address3_value"	INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_notes" (
	"id"	INTEGER,
	"page_id"	INTEGER,
	"x"	INTEGER,
	"y"	INTEGER,
	"text"	TEXT,
	"font_size"	INTEGER,
	"angle"	INTEGER DEFAULT 0,
	"type"	INTEGER DEFAULT 1,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_routes" (
	"id"	INTEGER,
	"page_id"	INTEGER,
	"name"	TEXT,
	"x"	INTEGER,
	"y"	INTEGER,
	"angle"	INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_response_modules" (
	"id"	INTEGER,
	"page_id"	INTEGER,
	"type"	INTEGER,
	"address"	INTEGER,
	"report_address"	INTEGER,
	"afterglow"	INTEGER,
	"x"	INTEGER,
	"y"	INTEGER,
	"angle"	INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_rails" (
	"id"	INTEGER,
	"page_id"	INTEGER,
	"left_control_id"	INTEGER,
	"right_control_id"	INTEGER,
	"left_outlet"	INTEGER,
	"right_outlet"	INTEGER,
	"value"	REAL,
	"left_response_module_id"	INTEGER DEFAULT 0,
	"right_response_module_id"	INTEGER DEFAULT 0,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_images" (
	"id"	INTEGER,
	"page_id"	INTEGER,
	"image_name"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_route_list" (
	"id"	INTEGER,
	"route_id"	INTEGER,
	"control_id"	TEXT,
	"state_id"	INTEGER,
	"position"	INTEGER,
	"wait_time"	INTEGER,
	"type"	INTEGER DEFAULT 0,
	"signal_id"	INTEGER DEFAULT 0,
	"signal_aspect"	INTEGER DEFAULT 0,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "vehicles_to_categories" (
	"id"	INTEGER NOT NULL,
	"vehicle_id"	INTEGER NOT NULL,
	"category_id"	INTEGER NOT NULL,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "categories" (
	"id"	INTEGER NOT NULL,
	"name"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "control_station_signals" (
	"id"	INTEGER NOT NULL,
	"page_id"	INTEGER,
	"x"	INTEGER,
	"y"	INTEGER,
	"angle"	REAL,
	"signal_id"	INTEGER,
	"signal_graph"	INTEGER,
	"address"	INTEGER,
	"active_aspects"	TEXT,
	"communication"	INTEGER,
	"z21_pro_link_id"	INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "paired_z21_pro_links" (
	"id"	INTEGER NOT NULL,
	"name"	TEXT,
	"mac"	TEXT,
	"last_seen_date"	DATE,
	"ip"	TEXT,
	"last_connected_device"	INTEGER,
	PRIMARY KEY("id")
);
COMMIT;
