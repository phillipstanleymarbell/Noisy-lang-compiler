dimensionTypeNames {
    time = "s";
    distance = "m";
    displacement = "m";
    mass = "g";
    angle = "rad";
}

vectorScalarPairs {
    displacement = distance;
    velocity = speed;
    acceleration = scalar_acceleration;
}

law {
    velocity = distance / time;
    acceleration = velocity / time;
    force = mass * acceleration;
    work = dot(force, displacement);
    torque = cross(force, displacement);
}

dimensionAliases {
    work = "J";
    force = "N";
}

vectorIntegrals {
    [displacement, velocity, acceleration];
}

scalarIntegrals {
    [distance, speed, scalar_acceleration];
}

