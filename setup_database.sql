-- GIS Map Application Database Setup
-- Run this script to create sample data for testing

-- Create sample polygons table
CREATE TABLE IF NOT EXISTS polygons (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    geom GEOMETRY(POLYGON, 4326),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create sample points table  
CREATE TABLE IF NOT EXISTS points (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    geom GEOMETRY(POINT, 4326),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert sample polygon data around Hanoi
INSERT INTO polygons (name, geom) VALUES 
('Hanoi Center', ST_GeomFromText('POLYGON((105.850 21.025, 105.850 21.035, 105.860 21.035, 105.860 21.025, 105.850 21.025))', 4326)),
('Hoan Kiem District', ST_GeomFromText('POLYGON((105.845 21.020, 105.845 21.040, 105.865 21.040, 105.865 21.020, 105.845 21.020))', 4326)),
('Ba Dinh District', ST_GeomFromText('POLYGON((105.830 21.030, 105.830 21.050, 105.850 21.050, 105.850 21.030, 105.830 21.030))', 4326)),
('Dong Da District', ST_GeomFromText('POLYGON((105.850 21.010, 105.850 21.030, 105.870 21.030, 105.870 21.010, 105.850 21.010))', 4326)),
('Hai Ba Trung District', ST_GeomFromText('POLYGON((105.860 21.000, 105.860 21.020, 105.880 21.020, 105.880 21.000, 105.860 21.000))', 4326))
ON CONFLICT DO NOTHING;

-- Insert sample point data (landmarks in Hanoi)
INSERT INTO points (name, geom) VALUES 
('Hoan Kiem Lake', ST_GeomFromText('POINT(105.8522 21.0285)', 4326)),
('Ho Chi Minh Mausoleum', ST_GeomFromText('POINT(105.8342 21.0366)', 4326)),
('Temple of Literature', ST_GeomFromText('POINT(105.8356 21.0227)', 4326)),
('Hanoi Opera House', ST_GeomFromText('POINT(105.8570 21.0200)', 4326)),
('Long Bien Bridge', ST_GeomFromText('POINT(105.8614 21.0454)', 4326)),
('Noi Bai Airport', ST_GeomFromText('POINT(105.8067 21.2187)', 4326)),
('Hanoi Railway Station', ST_GeomFromText('POINT(105.8417 21.0245)', 4326))
ON CONFLICT DO NOTHING;

-- Create spatial indexes for better performance
CREATE INDEX IF NOT EXISTS idx_polygons_geom ON polygons USING GIST (geom);
CREATE INDEX IF NOT EXISTS idx_points_geom ON points USING GIST (geom);

-- Grant permissions to postgres user
GRANT ALL PRIVILEGES ON TABLE polygons TO postgres;
GRANT ALL PRIVILEGES ON TABLE points TO postgres;
GRANT USAGE, SELECT ON SEQUENCE polygons_id_seq TO postgres;
GRANT USAGE, SELECT ON SEQUENCE points_id_seq TO postgres;

-- Display sample data
SELECT 'Polygons created:' as info, count(*) as count FROM polygons;
SELECT 'Points created:' as info, count(*) as count FROM points;

-- Show sample polygon data
SELECT id, name, ST_AsText(geom) as geometry FROM polygons LIMIT 3; 